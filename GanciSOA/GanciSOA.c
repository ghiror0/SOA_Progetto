#include <linux/module.h>
#include <linux/syscalls.h>

#include "moduleFunc/defineFile.c"
#include "moduleFunc/paramSys.c"
#include "moduleFunc/structSupport.c"
#include "moduleFunc/utility.c"
#include "moduleFunc/timeoutFunc.c"

////////////////////////////////////////////////DEFINIZIONE MODULO/////////////////////////////////////////////////////

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GANCI_VALERIO");
MODULE_DESCRIPTION("modulo per coda di messaggi");

#define MODNAME "MODULE_GANCI"
#define DEVICE_NAME "Device_Ganci"

static int dev_open(struct inode *, struct file *);

static int dev_release(struct inode *, struct file *);

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off);

static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param);

static int dev_flush(struct file *filp, fl_owner_t id);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define get_major(session)    MAJOR(session->f_inode->i_rdev)
#define get_minor(session)    MINOR(session->f_inode->i_rdev)

static int Major;

object_state objects[MINORS];

///////////////////////////////////////////CLOSE AND RELEASE///////////////////////////////////////////////////////////

static int dev_open(struct inode *inode, struct file *file) {
    int minor = get_minor(file);
    object_state *the_object;


    if (minor >= MINORS) {
        printk("%s: Minor number can't be handled\n\n", MODNAME);
        return -ENODEV;
    }

    printk("%s: device file successfully opened for object with minor %d\n", MODNAME,
           minor);

    the_object = objects + minor;

    //Lock necesserio per la sincronizzazione della open. si assicura che solo un utente alla volta possa controllare
    //il numero di utenti connessi. Si evita che 2 utenti che aprono la sessione in contemporanea possano inizializzare
    // 2 volte la struttura creando incongruenza
    LOCK(the_object->user_count_lock);

    if ((the_object->user_count == 0) && (the_object->head == NULL)) {
        init_object_state(the_object); //Si inizializza la struttura solo se necessario
    }else{
        the_object->user_count++;
    }

    UNLOCK(the_object->user_count_lock);

    return 0;
}

static int dev_release(struct inode *inode, struct file *file) {

    int minor = get_minor(file);

    object_state *the_object;
    the_object = objects + minor;


    LOCK(the_object->user_count_lock);

    if ((--(the_object->user_count) == 0) && (the_object->head == NULL)) {
        de_init_object_state(the_object); //Si dealloca la struttura in caso non ci siano ne utenti connessi ne messaggi
    }

    UNLOCK(the_object->user_count_lock);

    printk("%s: device file with minor number: %d -> closed\n", MODNAME, minor);  //device closed by default nop

    return 0;
}

/////////////////////////////////////////WRITE AND READ////////////////////////////////////////////////////////////////

static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

    int minor = get_minor(filp);
    int ret;
    mex_box *new_mex;


    object_state *the_object;
    the_object = objects + minor;

    printk("%s: somebody called a write on dev with [major,minor] number [%d,%d]\n", MODNAME,
           MAJOR(filp->f_inode->i_rdev), minor);

    //Controllo se il messaggio che voglio scrivere supera il limite di grandezza per il singolo messaggio
    if(len > max_message_size) {
        printk("%s: Message to large to be saved\n",MODNAME);
        return -EMEXSIZE;         //no space left on device
    }

    //////////////////////////Scrittura con timeout

    if(the_object->session_write_timeout > 0){
        if(timeout_write(the_object)<0){
            return -1;
        }
    }

    ////////////////////////////CHECK VALID BYTES//////////////
    //Controllo che ci sia abbastanza spazio per salvare il messaggio
    LOCK(valid_lock);

    if(len >  max_storage_size - valid_bytes ) {  //Non è presente abbastanza spazio

        UNLOCK(valid_lock);

        printk("%s: Not enough space\n",MODNAME);
        return -EOUTSPACE;    //out of stream resources
    }

    //Prenoto lo spazio necessario
    valid_bytes += len;

    UNLOCK(valid_lock);
    ///////////////////////////////////////////////////////////

    new_mex = vmalloc(sizeof(mex_box));

    new_mex->start = vmalloc(len);

    //ritorna il numero di bytes non letti con successo
    ret = copy_from_user(new_mex->start,buff,len);
    //aggiorno la lunghezza
    new_mex->len = len - ret;
    new_mex->next=NULL;

    //aggiorno il numero di bytes realmente salvati
    if(ret > 0){

        LOCK(valid_lock);

        valid_bytes -= ret ;

        UNLOCK(valid_lock);
    }

    insert_mex(the_object,new_mex);


    return len - ret;
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {

    int minor = get_minor(filp);
    int ret;
    mex_box *my_mex;

    object_state *the_object;
    the_object = objects + minor;


    printk("%s: somebody called a read on dev with [major,minor] number [%d,%d]\n",
           MODNAME, MAJOR(filp->f_inode->i_rdev), minor);


    if(the_object->session_read_timeout > 0){
        timeout_read(the_object);
    }


    ////////////////////////////////////////Prendo messaggio da leggere
    LOCK(the_object->head_lock);

    //Controllo se ci sono messaggi presenti
    if(the_object->head==NULL){
        UNLOCK(the_object->head_lock);
        printk("READ: Nessun messaggio in coda\n\n");
        return -1;
    }

    my_mex = the_object->head;

    //Aggiorno la lista dei messaggi eliminando quello preso
    the_object->head = my_mex->next;

    UNLOCK(the_object->head_lock);

    ////////////////////////////////////////////////////

    //Check sulla dimensione di bytes da leggere
    if(len > my_mex->len){
        len = my_mex->len;
    }

    ret = copy_to_user(buff,my_mex->start,len);

    vfree(my_mex->start);

    /////////////////////////////////////////Aggiornamento bytes validi
    LOCK(valid_lock);

    valid_bytes -= my_mex->len;

    UNLOCK(valid_lock);
    /////////////////////////////////////////Fine aggiornamento

    vfree(my_mex);

    return len - ret;
}

/////////////////////////////////////////IOCTL ADN FLUSH///////////////////////////////////////////////////////////////

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param) {

    int minor = get_minor(filp);

    object_state *the_object;
    the_object = objects + minor;

    printk("%s: somebody called an ioctl on dev with [major,minor] number [%d,%d] with command %u and param %lu\n",
           MODNAME, get_major(filp), get_minor(filp), command, param);

    switch(command)
    {
        case SET_SEND_TIMEOUT:
            the_object->session_write_timeout = param;
            printk("%s: IOCTL command 'SET_SEND_TIMEOUT': value of write_timeout for minor %d: %lu\n", MODNAME,minor, the_object->session_write_timeout);
            break;

        case SET_RECV_TIMEOUT:
            the_object->session_read_timeout = param;
            printk("%s: IOCTL command 'SET_RECV_TIMEOUT': value of read_timeout for minor %d: %lu\n", MODNAME,minor, the_object->session_read_timeout);
            break;

        case REVOKE_DELAYED_MESSAGES:
            printk("%s: IOCTL command 'SET_RECV_TIMEOUT' for minor %d\n", MODNAME,minor);
            revoke_delayed_write(the_object->write_queue_head);
            break;

        case REVOKE_DELAYED_MESSAGES_READ:
            printk("%s: IOCTL command 'REVOKE_DELAYED_MESSAGES_READ' for minor %d\n", MODNAME,minor);
            revoke_delayed_read(the_object->read_queue_head);
            break;

        case  REMOVE_MESSAGES:
            printk("%s: IOCTL command 'REMOVE_MESSAGES' for minor %d\n", MODNAME,minor);
            remove_mex(the_object);
            break;

        default:
            printk("%s: Invalid command on ioctl operation\n\n", MODNAME);
            break;
    }

    return 0;
}

static int dev_flush(struct file *filp, fl_owner_t id) {

    int minor = get_minor(filp);

    object_state *the_object;
    the_object = objects + minor;

    printk("%s: somebody called a flush on dev with [major,minor] number [%d,%d]\n",
           MODNAME, get_major(filp), get_minor(filp));

    ///////////////////////Interrompe le scritture pendenti
    revoke_delayed_write(the_object->write_queue_head);

    ///////////////////////Interrompe le letture pendenti
    revoke_delayed_read(the_object->read_queue_head);

    ///////////////////////Rimuovo i messaggi dalla coda
    //remove_mex(the_object);     // in caso si voglia resettare completamente lo stato del sistema

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .write = dev_write,
        .read = dev_read,
        .open =  dev_open,
        .release = dev_release,
        .unlocked_ioctl = dev_ioctl,
        .flush = dev_flush
};

///////////////////////////////////////////////INIT AND CLEANUP////////////////////////////////////////////////////////

int init_module(void) {

    int i;
    INIT_LOCK(valid_lock);
    valid_bytes = 0;


    Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);

    if (Major < 0) {
        printk(KERN_ERR "%s: registering device failed\n", MODNAME);
        return Major;
    }

    //Mi assicuro che la memoria sia pulita
    memset(objects,0,sizeof(object_state)*MINORS);

    //Inizializzo il lock per il conteggio degli utenti
    for(i = 0; i < MINORS;i++){
        INIT_LOCK(objects[i].user_count_lock);
    }


    printk(KERN_INFO
    "%s: new device registered, it is assigned major number %d\n", MODNAME, Major);

    return 0;
}

void cleanup_module(void) {
    int i;
    mex_box *temp;

    unregister_chrdev(Major, DEVICE_NAME);

    //Elimino tutti i messaggi pendenti
    for(i=0; i < MINORS; i++){  //Ciclo su tutti i minor numbers

        //Ciclo per tutti i messaggi presenti in coda
        while(objects[i].head != NULL){
            temp = objects[i].head;
            objects[i].head = objects[i].head->next;
            vfree(temp->start);
            vfree(temp);
        }
        //Elimino il lock per il conteggio utentui
        DESTROY_LOCK(objects[i].user_count_lock);

    }

    DESTROY_LOCK(valid_lock);

    printk(KERN_INFO
    "%s: new device unregistered, it was assigned major number %d\n", MODNAME, Major);

    return;
}
