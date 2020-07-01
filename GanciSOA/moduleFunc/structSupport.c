#include "defineFile.c"

typedef struct _mex_box {
    char *start;            //Posizione dei bytes salvati
    unsigned long len;      //Lunghezza messaggio
    struct _mex_box *next;

} mex_box;

typedef struct _queue_support{
    int awake;
    struct _queue_support *next;
    struct _queue_support *prec;  //Presente per permettere il risveglio di messaggi in mezzo alla coda
}queue;

typedef struct _object_state {


    lock_t user_count_lock;    //Lock per il numero di user attualmente connessi
    lock_t head_lock;          //Lock per il cambio del valore di head o tail

    mex_box *head;
    mex_box *tail;

    unsigned long user_count;            //Numero di utenti attualmente connessi
    long session_write_timeout;
    long session_read_timeout;

    ////////////////////////////Queue per risvegliare

    lock_t write_lock;   //Lock per la modifica del queue_support per le write
    lock_t read_lock;    //Lock per la modifica del queue_support per le read

    queue *write_queue_head;  //Inizio del supporto alla coda per la WriteWq
    queue *read_queue_head;   //Inizio del supporto alla coda per la WriteWq
} object_state;
