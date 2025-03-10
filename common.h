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
#include <time.h>
#include <ctype.h>

// Plik wspólnych definicji

#define PROJECT_PATH "./"
#define PROJECT_ID 'S'

#define FIRE_SIGNAL SIGUSR1

#define MAX_TABLES 100
#define MAX_CLIENTS 100

// 1) Zapytanie o stolik (klient -> kasjer)
struct msgbuf_request {
    long mtype;       // = 1
    int groupId;      // unikalny ID grupy w obrębie procesu klienta
    int groupSize;    // 1..3
    pid_t pidClient;  // PID procesu-klienta
};

// 2) Odpowiedź kasjera (mtype=2)
struct msgbuf_response {
    long mtype;       
    bool canSit;      // czy udało się posadzić
    int tableSize;    // rozmiar stolika (1..4)
    int tableIndex;   // który konkretnie stolik
    int groupId;      // ID grupy (echo z requestu)
};

// 3) Zwolnienie stolika (klient -> kasjer)
struct msgbuf_release {
    long mtype;       
    int groupId;      
    int tableSize;    
    int tableIndex;   
    int groupSize;    
    pid_t pidClient;
};

// 4) Rejestracja spawnera (mtype=10)
struct msgbuf_spawnerReg {
    long mtype;      // = 10
    pid_t spawnerPid;
};

#endif
