//Struttura per lavorare con le workqueue usando i container_of()
typedef struct _packed_work{
    struct delayed_work the_work;
    struct _write_queue* me;
    object_state *the_object;
    mex_box *mex;
} packed_work;

//Struttura per mantenere la lista dei work pendenti
typedef struct _write_queue{
    struct delayed_work *the_work;
    struct _write_queue *next;
    struct _write_queue *prec;
}write_queue;

struct workqueue_struct *writeWorkQueue;

//Crea ed inserisce il processo nella coda per il risveglio per mantenere la lista dei work attualmente pendenti
write_queue *insert_in_queue_write(object_state *the_object, struct delayed_work *the_work){
    write_queue *me;

    me = kmalloc(sizeof(write_queue),GFP_KERNEL);

    if(me==NULL){
        printk("Insert_in_queue_write: errore con allocazione\n");
        return NULL;
    }

    me->the_work = the_work;
    me->next = NULL;
    me->prec = NULL;

    LOCK(the_object->write_lock);

    if(the_object->write_queue_head == NULL){
        the_object->write_queue_head = me;
    }else{
        me->next = the_object->write_queue_head;
        the_object->write_queue_head->prec = me;
        the_object->write_queue_head = me;
    }

    UNLOCK(the_object->write_lock);

    return me;
}

//Elimina il processo dalla coda per il risveglio
void delete_queue_write(write_queue *me, object_state *the_object, bool delete){

    LOCK(the_object->write_lock);

    if(me->prec == NULL){  //Sono la testa
        the_object->write_queue_head = me->next;

        if(me->next != NULL){
            the_object->write_queue_head ->prec = NULL;

        }

    }else if(me->next == NULL){  //Sono la coda
        me->prec->next = NULL;

    } else{
        me->prec->next = me->next;
        me->next->prec = me->prec;
    }

    UNLOCK(the_object->write_lock);

    //Controllo se sono entrato per cancellare il work a causa di un comando ioctl o
    // se sto eliminando il work dalla mia queue
    if(delete){

        //elimino il work dalla workqueue
        if(cancel_delayed_work(me->the_work)){
            module_put(THIS_MODULE);
        }else{
            printk("Il work non era pendente ma siamo cmq entrati per cancellarlo\n");
        }
    }

    kfree(me);
}

//Rimuovo tutte le scritture pendenti
void revoke_delayed_write(object_state *the_object){
    write_queue *temp;

    temp = the_object->write_queue_head;

    while(temp != NULL){
        delete_queue_write(temp, the_object, 1);
        temp = the_object->write_queue_head;
    }

    return;
}

//Inizializzo la workqueue per il modulo
void initWorkQueue(void){
    char *workName = "writeWorkQueue";
    writeWorkQueue = create_workqueue(workName);
}

//work che si occupa di inserire il messaggio nella coda
void workQueue_write(void* data){
    object_state *the_object;
    mex_box* mex;
    int ret;

    //Rimuovo il work dalla coda dei work pendenti
    delete_queue_write(container_of(data,packed_work,the_work)->me,container_of(data,packed_work,the_work)->the_object,0);


    //recupero i dati tramite i container_of()
    the_object = container_of(data,packed_work,the_work)->the_object;
    mex=container_of(data,packed_work,the_work)->mex;


    ret=checkAndTakeSpace(mex->len);

    if(ret<0){
        kfree(mex->start);
        kfree(mex);
        module_put(THIS_MODULE);
        return ;
    }

    //Inserisco il messaggio nella coda
    insert_mex(the_object,mex);

    //Diminuisco il conteggio dei collegamenti al modulo
    module_put(THIS_MODULE);

}

//Inizializza la struttura con le informazioni per il work
packed_work* initTask(object_state *the_object, mex_box* mex){
    packed_work *the_task;

    the_task = kzalloc(sizeof(packed_work),GFP_ATOMIC);

    the_task->mex = mex;
    the_task->the_object = the_object;

    return the_task;
}

//Funzione che inizializza il work per la workQueue
int deferredWrite(object_state *the_object, size_t len, const char *buff){
    packed_work *the_task;
    mex_box* mex;

    //Aumento il numero di collegamenti al modulo per il work che andro' a creare
    if(!try_module_get(THIS_MODULE)) {
        printk("DeferredWrite: Errore nel try_module_get()\n");
        return -1;
    }

    //Creo una copia del messaggio
    mex = make_mex(len,buff);

    if (mex==NULL) {
        printk("DeferredWrite: Errore nel make_mex()\n");
        return -1;
    }

    //Inizializzo la struttura che contiene le informazioni per il work (ed il work stesso)
    the_task = initTask(the_object,mex);

    if (the_task==NULL) {
        printk("DeferredWrite: Errore nel initTask()\n");
        return -1;
    }

    INIT_DELAYED_WORK( &(the_task->the_work), (void*)workQueue_write);

    //Memorizzo nella struttura l'indirizzo al work nella coda per il risveglio
    the_task->me = (write_queue *)insert_in_queue_write(the_object,&(the_task->the_work));

    //inserisco il work nella work queue specificando il delay
    if(queue_delayed_work(writeWorkQueue,&the_task->the_work,the_object->session_write_timeout)==0){
        printk("DeferredWrite: Errore nel queue_delayed_work()\n");
        return -1;
    }

    return 0;
}

