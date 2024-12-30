#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <wait.h>

#define PROJECT_PATH "./" 
#define PROJECT_ID 'S'    

#define MAX_GROUP_SIZE 3

typedef struct {
    int free_1_person_tables; 
    int free_2_person_tables;
    int free_3_person_tables;
    int free_4_person_tables;
} TablesState;


struct msgbuf_request {
    long mtype;  // typ wiadomości: 1 = zapytanie od klienta, 2 = odpowiedź kasjera
    int groupSize;
    int pidClient; // PID procesu-klienta (do ewentualnej identyfikacji)
};

struct msgbuf_response {
    long mtype; 
    bool canSit;
    int tableSize;  // 1,2,3,4 - jaki stolik przydzielony(lub 0 jeśli brak)
};

#define FIRE_SIGNAL SIGUSR1

#endif

