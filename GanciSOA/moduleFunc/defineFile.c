#include "defineString.c"

#define MODNAME "MODULE_GANCI"
#define DEVICE_NAME "Device_Ganci"

#define get_major(session)    MAJOR(session->f_inode->i_rdev)
#define get_minor(session)    MINOR(session->f_inode->i_rdev)

#define MAX_STORAGE 10000
#define MAX_MESSAGE 100

#define SET_SEND_TIMEOUT 1
#define SET_RECV_TIMEOUT 3
#define REVOKE_DELAYED_MESSAGES 5
#define REVOKE_DELAYED_MESSAGES_READ 6
#define REMOVE_MESSAGES 7


#define MINORS 3      //Numero massimo di sessioni contemporanee consentite

#define EMEXSIZE -3   //Il messaggio che si vuole scrivere supera la dimensione massima consentita da MAX_MESSAGE
#define EOUTSPACE -4  //Il numero di bytes che si vuole scrivere supera il numero di bytes liberi disponibili
#define REVOKE -5     //L'operazione è stata revocata


//////////////////////////////Sincronizzazione

#define MUTEX 0  //se 0 utilizza spinlock, se 1 utilizza mutex

#if MUTEX
#define lock_t struct mutex
#define LOCK(lock) mutex_lock(&lock)
#define UNLOCK(lock) mutex_unlock(&lock)
#define INIT_LOCK(lock) mutex_init(&lock)
#define DESTROY_LOCK(lock) mutex_destroy(&lock)

#else
#define lock_t spinlock_t
#define LOCK(lock) spin_lock(&lock)
#define UNLOCK(lock) spin_unlock(&lock)
#define INIT_LOCK(lock) spin_lock_init(&lock)
#define DESTROY_LOCK(lock)
#endif

int valid_bytes;     //Numero di bytes ancora disponibili
lock_t valid_lock;   //Lock per il cambio del valore di valid_bytes



