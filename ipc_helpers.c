#include "common.h"
#include "ipc_helpers.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

int create_msg_queue(key_t key){
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Błąd: Nie udało się utworzyć kolejki komunikatów za pomocą msgget.");
        exit(EXIT_FAILURE);
    }
    return msgid;
}

void remove_msg_queue(int msgid){
    if(msgctl(msgid, IPC_RMID, NULL) == -1){
        perror("Błąd: Nie udało się usunąć kolejki komunikatów za pomocą msgctl.");
    }
}

int get_msg_queue(key_t key){
    int msgid = msgget(key, 0666);
    if(msgid == -1){
        perror("Błąd: Nie udało się uzyskać identyfikatora kolejki komunikatów za pomocą msgget.");
        exit(EXIT_FAILURE);
    }
    return msgid;
}
