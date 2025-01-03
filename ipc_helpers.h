#ifndef IPC_HELPERS_H
#define IPC_HELPERS_H

#include "common.h"


int create_msg_queue(key_t key);
int create_semaphore(key_t key);