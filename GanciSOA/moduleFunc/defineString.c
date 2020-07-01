#define IOCTL_INFO_STRING "Selezionato comando ioctl: "

#define SET_SEND_TIMEOUT_INFO IOCTL_INFO_STRING "'SET_SEND_TIMEOUT'"

#define SET_RECV_TIMEOUT_INFO IOCTL_INFO_STRING  "'SET_RECV_TIMEOUT'"

#define REVOKE_DELAYED_MESSAGES_INFO IOCTL_INFO_STRING  "'REVOKE_DELAYED_MESSAGES'"

#define REVOKE_DELAYED_MESSAGES_READ_INFO IOCTL_INFO_STRING "'REVOKE_DELAYED_MESSAGES_READ'"

#define REMOVE_MESSAGES_INFO IOCTL_INFO_STRING "'REMOVE_MESSAGES'"

#define UNEXPECTED_IOCTL "Il comando ioctl non corrisponde a nessuno dei comandi conosciuti"

#define WRITE_CMD "write"
#define READ_CMD "read"
#define IOCTL_CMD "ioctl"
#define CLOSE_CMD "close"
#define HELP_CMD "help"
#define TEST_CMD "test"


#define HELP_STRING "Comandi accettati:\nwrite\nread\nioctl\nclose\ntest\nhelp"
#define UNEXPECTED_CMD "Il comando inserito non corrisponde a nessuno dei comandi conosciuti!"

#define NO_MEX "Non è presente nessun messaggio in coda!"
#define PRINT_READ_MEX "risultato lettura: "

#define IOCTL_REQUEST "Specificare il comando ioctl:"
#define IOCTL_REQUEST_PARAM "Inserire il valore del parametro del comando ioctl:"

#define EXIT_STRING "chiusura sessione!"

#define READ_REQUEST "Inserire il numero di bytes da leggere: "
#define WRITE_REQUEST "Inserire il testo: "

#define OPEN_STRING "apertura device: "
#define OPEN_SUCCESS "device aperto con successo!"
