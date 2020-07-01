#include "stddef.h"
#include "stdio.h"
#include <stdlib.h>

#define CD "cd /sys/module/GanciSOA/parameters"
#define CMD "sh -c 'echo "


int main(int argc, char **argv) {

    long param;
    unsigned long value;
    char buff[120];
    char* paramString;

    if (argc < 2) {
        printf("useg: prog param value");
        return -1;
    }

    param = strtol(argv[1], NULL, 10);
    value = strtol(argv[2], NULL, 10);

    switch(param){
        case 1:
            paramString = "max_message_size";
            break;
        case 2:
            paramString = "max_storage_size";
            break;
        default:
            printf("parametro non trovato\n");
            return -1;
    }

    sprintf(buff, "%s\n%s\"%lu\">>%s'", CD,CMD,value,paramString);

    system(buff);

    return 0;
}