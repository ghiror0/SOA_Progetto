#include "user.h"

//Valore numerico relativo alla sessione I/O aperta dall'utente
int fd;


ssize_t writeCmd() {

    char buffer[100];
    printf("%s\n", WRITE_REQUEST);
    scanf("%s", buffer);
    return write(fd, buffer, strlen(buffer) + 1); //Il +1 serve per inserire anche il finestringa
}

ssize_t readCmd() {
    char buffer[100];

    printf("%s\n", READ_REQUEST);

    scanf("%s", buffer);
    size_t size = strtol(buffer, NULL, 10);

    if(size>100){
        size=100;
    }

    ssize_t rd = read(fd, buffer, size);

    if (rd < 0) {
        printf("%s\n", NO_MEX);
    } else {
        printf("%s%s\n", PRINT_READ_MEX, buffer);
    }


    return rd;
}

int closeCmd() {

    close(fd);

    printf("\n%s\n\n", EXIT_STRING);
    exit(0);

}

void infoIoctl(int ioctl) {

    switch (ioctl) {

        case SET_SEND_TIMEOUT:
            printf("%s\n", SET_SEND_TIMEOUT_INFO);
            break;
        case SET_RECV_TIMEOUT:
            printf("%s\n", SET_RECV_TIMEOUT_INFO);
            break;
        case REVOKE_DELAYED_MESSAGES:
            printf("%s\n", REVOKE_DELAYED_MESSAGES_INFO);
            break;
        case REVOKE_DELAYED_MESSAGES_READ:
            printf("%s\n", REVOKE_DELAYED_MESSAGES_READ_INFO);
            break;
        case REMOVE_MESSAGES:
            printf("%s\n", REMOVE_MESSAGES_INFO);
            break;
        default:
            printf("%s\n", UNEXPECTED_IOCTL);
            break;
    }

}

int ioctlCmd() {
    char buffTemp[10];
    int command;
    unsigned int param;

    printf("%s\n", IOCTL_REQUEST);
    scanf("%s", buffTemp);
    command = (int) strtol(buffTemp, NULL, 10);

    infoIoctl(command);

    if (command < 4) {
        printf("%s\n", IOCTL_REQUEST_PARAM);
        scanf("%s", buffTemp);
        param = (int) strtol(buffTemp, NULL, 10);
    } else {
        param = 0;
    }

    ioctl(fd, command, param);
    return 0;
}

void help() {
    printf("%s\n", HELP_STRING);
}

void *threadWrite() {

    int rd;
    char buffer[30];
    sprintf(buffer, "%s%lu", "TEST", pthread_self());

    for (int i = 0; i < MAXWRITE; i++) {
        write(fd, buffer, strlen(buffer) + 1);  //Il +1 serve per inserire anche il finestringa
    }

    return NULL;
}

void *threadRead() {

    char buffer[30];
    int rd;

    for (int i = 0; i < MAXREAD; i++) {
        rd = read(fd, buffer, 30);

        if (rd < 0) {
            printf("TH %lu: %s\n", (long) pthread_self(), NO_MEX);
        } else {
            printf("TH %lu: %s %s\n", (long) pthread_self(), PRINT_READ_MEX, buffer);
        }

    }
    return NULL;
}

void testCmd() {
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

int checkCommand(char *cmd) {

    if (strcmp(cmd, WRITE_CMD) == 0) {
        writeCmd();

    } else if (strcmp(cmd, READ_CMD) == 0) {
        readCmd();

    } else if (strcmp(cmd, CLOSE_CMD) == 0) {
        closeCmd();

    } else if (strcmp(cmd, IOCTL_CMD) == 0) {
        ioctlCmd();

    } else if (strcmp(cmd, TEST_CMD) == 0) {
        testCmd();

    } else if (strcmp(cmd, HELP_CMD) == 0) {
        help();
    } else {
        printf("\n\n%s\n", UNEXPECTED_CMD);
        help();
    }

    return 0;
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

    printf("%s %s\n", OPEN_STRING, buff);

    fd = open(buff, O_RDWR);

    if (fd == -1) {
        printf("open error on device %s\n", buff);
        return 0;
    }

    printf("%s\n", OPEN_SUCCESS);

    char cmdBuff[10];

    while (1) {
        printf("\n\nInserire il comando:\n");
        scanf("%s", cmdBuff);
        checkCommand(cmdBuff);
    }

}
