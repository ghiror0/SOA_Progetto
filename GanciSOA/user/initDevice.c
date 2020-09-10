//
// Created by valerio on 28/05/20.
//

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysmacros.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "../moduleFunc/defineFIleUser.c"

#define PRINT 0

int main(int argc, char **argv) {

    unsigned long major;
    int fd;
    char *device = "device";
    char bufferName[10];

    if (argc < 2) {
        printf("useg: prog major");
        return -1;
    }

    major = strtol(argv[1], NULL, 10);


    for (int i = 0; i < MINORS; i++) {

        sprintf(bufferName, "%s%d", device, i);

        if (mknod(bufferName, S_IFCHR, makedev(major, i)) < 0) { //Creo il charDevice legato al majorNumber in input ed minor number i

            if (errno != EEXIST) {
                printf("Errore nel creare il nodo: %s\n", strerror(errno));
                return -1;
            }
        }

        if(PRINT) {
            printf("Device%d correttamente creato\n", i);
        }

        fd = open(bufferName, O_RDWR);

        if (fd < 0) {
            printf("Errore nell'aprire %s %s\n", bufferName, strerror(errno));
            return -1;
        }

        if(PRINT) {
            printf("Device%d correttamente aperto\n", i);
        }

        if (fchmod(fd, 0006) < 0) {
            printf("Errore nel modificare i permessi del nodo numero %d: %s\n", i, strerror(errno));
            return -1;
        }

        if(PRINT) {
            printf("Permessi del Device%d correttamente cambiati\n", i);
        }

        close(fd);

        if(PRINT) {
            printf("Device%d correttamente chiuso\n", i);
        }

    }

    return 0;
}
