#ifndef IPC_HELPERS_H
#define IPC_HELPERS_H

#include "common.h"


int create_msg_queue(key_t key);

int create_semaphore(key_t key);
int get_semaphore(key_t key);
void remove_semaphore(int semid);

void semaphore_down(int semid);
void semaphore_up(int semid);