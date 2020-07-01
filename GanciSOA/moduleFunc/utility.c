int valid_bytes;     //Numero di bytes ancora disponibili

lock_t valid_lock;   //Lock per il cambio del valore di valid_bytes

//Inizializza la struttura object_state
void init_object_state(object_state *the_object){

    INIT_LOCK(the_object->head_lock);

    the_object->user_count++;  //la funzione è chiamata quando

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


//Cancella TUTTI i messaggi nella coda  --> comando ioctl
void remove_mex(object_state *the_object){
    mex_box *my_mex;

    next:

    LOCK(the_object->head_lock);

    //Controllo se ci sono messaggi presenti
    if(the_object->head==NULL){
        UNLOCK(the_object->head_lock);
        return ;
    }

    my_mex = the_object->head;

    //Aggiorno la lista dei messaggi eliminando quello preso
    the_object->head = my_mex->next;

    UNLOCK(the_object->head_lock);

    vfree(my_mex->start);

    LOCK(valid_lock);
    valid_bytes-= my_mex->len;
    UNLOCK(valid_lock);

    vfree(my_mex);

    goto next;

}
