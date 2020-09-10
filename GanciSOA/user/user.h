//
// Created by valerio on 30/05/20.
//

#ifndef PROVA_USER_H
#define PROVA_USER_H

#endif //PROVA_USER_H


#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "../moduleFunc/defineFIleUser.c"

//Define inerenti al numero di scritture, e letture, che un thread deve eseguire ad ogni comando

#define MAXWRITE 5
#define MAXREAD 5

#define TEST 1000


//Funzione per richiedere una scrittura, chiede all'utente di inserire il testo da scrivere (max100 caratteri)
ssize_t writeCmd();

//Funzione per richiedere una lettura, chiede all'utente di inserire il numero di bytes da leggere (max100)
ssize_t readCmd();

//Funzione per la chiusura della session I/O con il device
int closeCmd();

//Funzione che analizza il parametro 'cmd' in input avviando il comando opportuno
int checkCommand(char *cmd);

//Funzione che mostra all'utente la tipologia di comando ioctl selezionata
void infoIoctl(int ioctl);

//Funzione che chiama un comando ioctl in base all'input dell'utente
int ioctlCmd();

//Funzione che mostra i comandi disponibili all'utente
void help();

//Funzione per scrittura tramite thread
void *threadWrite();

//Funzione per lettura tramite thread
void *threadRead();

//Funzione che si occupa di fare molteplici scritture e letture in contemporanea
void testCmd();
