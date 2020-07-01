//
// Created by valerio on 25/05/20.
//

DECLARE_WAIT_QUEUE_HEAD(WriteWq);
DECLARE_WAIT_QUEUE_HEAD(ReadWq);

//Crea ed inserisce il processo nella coda per il risveglio
queue *sleep_write(object_state *the_object){
    queue *me;

    me = vmalloc(sizeof(queue));

    if(me==NULL){
        printk("WRITE: Errore nella vmalloc del supporto alla coda\n\n");
        return NULL;
    }

    me->awake = 0;
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
void wake_write(queue *me, object_state *the_object){

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
    vfree(me);
}

//Crea ed inserisce il processo nella coda per il risveglio
queue *sleep_read(object_state *the_object){
    queue *me;

    me = vmalloc(sizeof(queue));

    me->awake = 0;
    me->next = NULL;
    me->prec = NULL;

    LOCK(the_object->read_lock);

    if(the_object->read_queue_head == NULL){
        the_object->read_queue_head = me;
    }else{
        me->next = the_object->read_queue_head;
        the_object->read_queue_head = me;
        me->next->prec = me;
    }

    UNLOCK(the_object->read_lock);
    return me;
}

//Elimina il processo dalla coda per il risveglio
void wake_read(queue *me, object_state *the_object){

    LOCK(the_object->read_lock);

    if(me->prec == NULL){  //Sono la testa
        the_object->read_queue_head = me->next;
        if(me->next != NULL){
            me->next->prec = NULL;
        }

    } else if(me->next == NULL) { //Sono la coda
        me->prec->next = NULL;

    } else{
        me->prec->next = me->next;
        me->next->prec = me->prec;
    }

    vfree(me);

    UNLOCK(the_object->read_lock);
}

//Mette Il processo in attesa sulla WriteWq con relative operazioni sul supporto alla coda
int timeout_write(object_state *the_object){
    queue *me;
    int revoke;

    me = sleep_write(the_object);

    if(me == NULL){
        printk("WRITE(dopo sleep_write): Errore nella vmalloc del supporto alla coda\n\n");
        return -1;
    }

    revoke = 0;

    if(wait_event_timeout(WriteWq,me->awake == 1,the_object->session_write_timeout) > 0){

        revoke = REVOKE;
    }

    wake_write(me,the_object);

    return revoke;
}

//Mette Il processo in attesa sulla ReadWq con relative operazioni sul supporto alla coda
int timeout_read(object_state *the_object){
    queue *me;
    int revoke;

    me = sleep_read(the_object);

    revoke = 0;

    if(wait_event_timeout(ReadWq,me->awake == 1,the_object->session_read_timeout) > 0){

        revoke = REVOKE;
    }

    wake_read(me,the_object);

    return revoke;
}



//Sveglia tutti i processi sulla coda WriteWq attraverso l'uso del supporto alla coda
void revoke_delayed_write(queue* head){
    queue *temp;

    temp = head;

    while(temp != NULL){
        temp->awake = 1;
        temp = temp->next;
    }

    wake_up_all(&WriteWq);

    return;
}

//Sveglia tutti iprocessi sulla coda ReadWq attraverso l'uso del supporto alla coda
void revoke_delayed_read(queue* head){
    queue *temp;

    temp = head;

    while(temp != NULL){
        temp->awake = 1;
        temp = temp->next;
    }

    wake_up_all(&ReadWq);

    return;
}

