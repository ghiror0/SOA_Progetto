//
// Created by valerio on 07/08/20.
//

#include "defineString.c"

#define MODNAME "MODULE_GANCI"
#define DEVICE_NAME "Device_Ganci"

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

