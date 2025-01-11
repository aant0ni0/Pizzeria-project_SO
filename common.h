#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <wait.h>
#include <pthread.h>

#define PROJECT_PATH "./"
#define PROJECT_ID 'S'

#define FIRE_SIGNAL SIGUSR1

#define MAX_GROUP_SIZE 3

// Stan pizzerii (zarządzany wyłącznie przez kasjera):
typedef struct {
    int free_1_person_tables;
    int free_2_person_tables;
    int free_3_person_tables;
    int free_4_person_tables;

    int half_occupied_2_person_tables;
    int half_occupied_4_person_tables;
} TablesState;

// Typy komunikatów: 1 => zapytanie o stolik, 2 => odpowiedź, 3 => zwolnienie
struct msgbuf_request {
    long mtype;       // = 1
    int groupId;      // unikalny ID grupy w obrębie procesu klienta
    int groupSize;    // 1..3
    pid_t pidClient;  // PID procesu klienta
};

struct msgbuf_response {
    long mtype;       // = 2
    bool canSit;
    int tableSize;    // 1,2,3,4 lub 0
    int groupId;      // żeby klient wiedział, której grupy dotyczy
};

struct msgbuf_release {
    long mtype;       // = 3
    int groupId;
    int tableSize;
    int groupSize;
    pid_t pidClient;
};

#endif
