//Inizializza la struttura object_state
void init_object_state(object_state *the_object){

    INIT_LOCK(the_object->head_lock);
    INIT_LOCK(the_object->write_lock);
    INIT_LOCK(the_object->read_lock);

    return;
}

//Rimuove i mutex dalla struttura
void de_init_object_state(object_state *the_object){

    DESTROY_LOCK(the_object->head_lock);
    DESTROY_LOCK(the_object->read_lock);
    DESTROY_LOCK(the_object->write_lock);

    return;
}

//Inserisce un mex_box nella lista dei messaggi
void insert_mex(object_state *the_object, mex_box *new_mex){

    LOCK(the_object->head_lock);

    if(the_object->head == NULL){  //Inserimento in testa
        the_object->head = new_mex;
        the_object->tail = new_mex;
    }else{                         //Inserimento in coda
        the_object->tail->next = new_mex;
        the_object->tail = new_mex;
    }
    UNLOCK(the_object->head_lock);

    return;
}

//Cancella il primo messaggio dalla testa della coda
int remove_head_mex(object_state *the_object){
    mex_box *my_mex;

    LOCK(the_object->head_lock);

    //Controllo se ci sono messaggi presenti
    if(the_object->head==NULL){
        UNLOCK(the_object->head_lock);
        return -1;
    }

    my_mex = the_object->head;

    //Aggiorno la lista dei messaggi eliminando quello preso
    the_object->head = my_mex->next;

    UNLOCK(the_object->head_lock);

    kfree(my_mex->start);

    LOCK(valid_lock);
    valid_bytes-= my_mex->len;
    UNLOCK(valid_lock);

    kfree(my_mex);

    return 0;
}

//Cancella TUTTI i messaggi nella coda  --> comando ioctl
void remove_all_mex(object_state *the_object){
    int ret;

    do{
        ret = remove_head_mex(the_object);
    }
    while(ret==0);
}

//Controlla che ci sia lo spazione necessario per il messaggio e ne sottrae quanto necessario
int checkAndTakeSpace(size_t len){
    LOCK(valid_lock);

    if(len >  max_storage_size - valid_bytes ) {  //Non è presente abbastanza spazio

        UNLOCK(valid_lock);

        printk("%s: Not enough space\n",MODNAME);
        return EOUTSPACE;    //out of stream resources
    }

    //Prenoto lo spazio necessario
    valid_bytes += len;
    UNLOCK(valid_lock);

    return 0;
}

//aggiorno il valore di bytes validi
void updateValidBytes(size_t ret){
    if(ret > 0){

        LOCK(valid_lock);
        valid_bytes -= ret ;
        UNLOCK(valid_lock);
    }
}

//Crea il mex_box, alloca la memoria ed inserisce il testo
mex_box* make_mex(size_t len, const char *buff){
    int ret;
    mex_box *new_mex;

    new_mex = kmalloc(sizeof(mex_box),GFP_KERNEL);
    new_mex->start = kmalloc(len,GFP_KERNEL);

    //ritorna il numero di bytes non letti con successo
    ret = copy_from_user(new_mex->start,buff,len);

    //aggiorno la lunghezza
    new_mex->len = len - ret;

    new_mex->next=NULL;

    return new_mex;
}

//Prende il messaggio in testa
int take_mex(object_state *the_object,mex_box **mex){
    mex_box *my_mex;

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

    *mex = my_mex;

    return 0;
}