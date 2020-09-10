//Funzione che crea ed inserisce il messaggio nella coda
size_t actual_write(object_state *the_object, size_t len, const char *buff){
    mex_box *new_mex = NULL;
    int ret;

    ////////////////////////////////////Controllo disponibilita' spazio

    //Controllo e prenoto lo spazio
    ret=checkAndTakeSpace(len);

    if(ret<0){
        printk("dentro/dopo check and take space\n\n");
        return ret;
    }

    ////////////////////////////////////Inserisco il testo nel messaggio

    //Creazione messaggio
    new_mex = make_mex(len,buff);

    ret = len - new_mex->len;

    //Aggiornamento dei bytes usati
    updateValidBytes(ret);

    //messaggio inserito in coda
    insert_mex(the_object,new_mex);

    return len - ret;
}

//Funzione che recupera, ed elimina, il primo messaggio della coda
size_t actual_read(object_state *the_object, size_t len, char *buff){

    int ret;
    mex_box *my_mex=NULL;

    //Lettura con timeout
    if(the_object->session_read_timeout>0){
        timeout_read(the_object);
    }

    //preso messaggio in testa alla coda
    ret = take_mex(the_object,&my_mex);

    if(ret<0){
        return ret;
    }

    //Check sulla dimensione di bytes da leggere
    if(len > my_mex->len){
        len = my_mex->len;
    }

    ret = copy_to_user(buff,my_mex->start,len);
    kfree(my_mex->start);

    /////////////////////////////////////////Aggiornamento bytes validi
    LOCK(valid_lock);
    valid_bytes -= my_mex->len;
    UNLOCK(valid_lock);
    /////////////////////////////////////////Fine aggiornamento

    kfree(my_mex);

    return len - ret;
}


