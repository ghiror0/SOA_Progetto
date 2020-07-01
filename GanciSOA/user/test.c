//
// Created by valerio on 18/06/20.
//

#include "user.h"

#define PRINT 1

//Valore numerico relativo alla sessione I/O aperta dall'utente
int fd;


void *threadWrite() {


    char buffer[30];


    for (int i = 0; i < TEST; i++) {
        sprintf(buffer, "%d%s%lu",i, "TEST", pthread_self());
        write(fd, buffer, strlen(buffer) + 1);  //Il +1 serve per inserire anche il finestringa
    }

    return NULL;
}

void *threadRead() {

    char buffer[30];
    int rd;

    for (int i = 0; i < TEST; i++) {
        rd = read(fd, buffer, 30);

        if(PRINT){
        if (rd < 0) {
            printf("TH %lu: %s\n", (long) pthread_self(), NO_MEX);
        } else {
            printf("TH %lu: %s %s\n", (long) pthread_self(), PRINT_READ_MEX, buffer);
        }
        }

    }
    return NULL;
}

void test(){

    pthread_t tid1, tid2, tid3, tid4;

    //Creazione thread
    pthread_create(&tid1, NULL, threadWrite, NULL);
    pthread_create(&tid2, NULL, threadRead, NULL);
    pthread_create(&tid3, NULL, threadWrite, NULL);
    pthread_create(&tid4, NULL, threadRead, NULL);

    //Attesa thread
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);

}

int main(int argc, char **argv) {

    unsigned long minor;
    char buff[100];

    if (argc < 1) {
        printf("useg: prog minor");
        return -1;
    }

    minor = strtol(argv[1], NULL, 10);

    sprintf(buff, "device%lu", minor);

    fd = open(buff, O_RDWR);

    if (fd == -1) {
        printf("open error on device %s\n", buff);
        return 0;
    }

    test();

}