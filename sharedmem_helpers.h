#ifndef SHAREDMEM_HELPERS_H
#define SHAREDMEM_HELPERS_H

#include "common.h"


int create_shared_memory(key_t key);
TablesState* attach_shared_memory(int shmid);