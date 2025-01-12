#ifndef IPC_HELPERS_H
#define IPC_HELPERS_H

#include "common.h"


int create_msg_queue(key_t key);
void remove_msg_queue(int msgid);
int get_msg_queue(key_t key);



#endif