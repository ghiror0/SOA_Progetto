#include "moduleFunc/module.h"

////////////////////////////////////////////////DEFINIZIONE MODULO/////////////////////////////////////////////////////

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GANCI_VALERIO");
MODULE_DESCRIPTION("modulo per coda di messaggi");


static int dev_open(struct inode *, struct file *);

static int dev_release(struct inode *, struct file *);

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off);

static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static long dev_ioctl(struct file *filp, unsigned int command, unsigned long param);

static int dev_flush(struct file *filp, fl_owner_t id);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int Major;

object_state objects[MINORS];

///////////////////////////////////////////CLOSE AND RELEASE///////////////////////////////////////////////////////////

static int dev_open(struct inode *inode, struct file *file) {
    int minor = get_minor(file);

    if (minor >= MINORS) {
        printk("%s: Minor number can't be handled\n\n", MODNAME);
        return ENODEV;
    }

    printk("%s: device file successfully opened for object with minor %d\n", MODNAME,
           minor);

    return 0;
}

static int dev_release(struct inode *inode, struct file *file) {

    int minor = get_minor(file);
    printk("%s: device file with minor number: %d -> closed\n", MODNAME, minor);  //device closed by default nop

    return 0;
}

/////////////////////////////////////////WRITE AND READ////////////////////////////////////////////////////////////////

static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {

    int minor = get_minor(filp);
    object_state *the_object = objects + minor;

    printk("%s: somebody called a write on dev with [major,minor] number [%d,%d]\n", MODNAME,
           MAJOR(filp->f_inode->i_rdev), minor);

    //Controllo se il messaggio che voglio scrivere supera il limite di grandezza per il singolo messaggio
    if(len > max_message_size) {
        printk("%s: Message to large to be saved\n",MODNAME);
        return EMEXSIZE;         //no space left on device
    }

    if(the_object->session_write_timeout>0){
        return deferredWrite(the_object,len,buff);
    }

    return actual_write(the_object,len,buff);
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {

    int minor = get_minor(filp);
    object_state *the_object = objects + minor;

    printk("%s: somebody called a read on dev with [major,minor] number [%d,%d]\n",
           MODNAME, MAJOR(filp->f_inode->i_rdev), minor);

    return actual_read(the_object, len, buff);

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
            revoke_delayed_write(the_object);
            break;

        case REVOKE_DELAYED_MESSAGES_READ:
            printk("%s: IOCTL command 'REVOKE_DELAYED_MESSAGES_READ' for minor %d\n", MODNAME,minor);
            revoke_delayed_read(the_object->read_queue_head);
            break;

        case  REMOVE_MESSAGES:
            printk("%s: IOCTL command 'REMOVE_MESSAGES' for minor %d\n", MODNAME,minor);
            remove_all_mex(the_object);
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
    revoke_delayed_write(the_object);

    ///////////////////////Interrompe le letture pendenti
    revoke_delayed_read(the_object->read_queue_head);

    ///////////////////////Rimuovo i messaggi dalla coda
    //remove_all_mex(the_object);     // in caso si voglia resettare completamente lo stato del sistema

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

    Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);

    if (Major < 0) {
        printk(KERN_ERR "%s: registering device failed\n", MODNAME);
        return Major;
    }

    INIT_LOCK(valid_lock);
    valid_bytes = 0;

    //Mi assicuro che la memoria sia pulita
    memset(objects,0,sizeof(object_state)*MINORS);

    //Inizializzo il lock per il conteggio degli utenti
    for(i = 0; i < MINORS;i++){
        init_object_state(objects+i);
    }

    initWorkQueue();

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
            kfree(temp->start);
            kfree(temp);
        }
        de_init_object_state(objects+i);
    }


    DESTROY_LOCK(valid_lock);
    destroy_workqueue(writeWorkQueue);

    printk(KERN_INFO
    "%s: new device unregistered, it was assigned major number %d\n", MODNAME, Major);

    return;
}