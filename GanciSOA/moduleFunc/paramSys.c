#include <linux/moduleparam.h>
#include <linux/sysfs.h>

int max_message_size = MAX_MESSAGE;
module_param(max_message_size,int,0644);
//MODULE_PARM_DESC(max_message_size, "Number of bytes you can store with a single write");

int max_storage_size = MAX_STORAGE;
module_param(max_storage_size,int,0644);
//MODULE_PARM_DESC(max_storage_size,"Number of bytes you can store on this device");



