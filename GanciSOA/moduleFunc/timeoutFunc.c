DECLARE_WAIT_QUEUE_HEAD(ReadWq);

//Crea ed inserisce il processo nella coda per il risveglio
queue *sleep_read(object_state *the_object){
    queue *me;

    me = kmalloc(sizeof(queue),GFP_KERNEL);

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

    kfree(me);

    UNLOCK(the_object->read_lock);
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

