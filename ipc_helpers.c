#include "ipc_helpers.h"


int create_msg_queue(key_t){
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Błąd: Nie udało się utworzyć kolejki komunikatów za pomocą msgget. Upewnij się, że używany klucz IPC jest poprawny i że proces ma wystarczające uprawnienia do tworzenia zasobów IPC.");
        exit(EXIT_FAILURE);
    }
    return msgid;
}

int create_semaphore(key_t key){
    int semid = semget(key, 1, IPC_CREAT | 0666);
    if(semid ==-1){
        perror("Błąd: Nie udało się utworzyć semafora za pomocą semget. Upewnij się, że używany klucz IPC jest poprawny oraz że system ma wystarczającą liczbę dostępnych zasobów IPC.")
        exit(EXIT_FAILURE);
    }
    
    if(semctl(semid, 0, SETVAL, 1)==-1){
        perror("Błąd: Nie udało się ustawić wartości początkowej semafora za pomocą semctl. Sprawdź, czy system ma dostępne zasoby IPC oraz czy proces ma wystarczające uprawnienia.");
        exit(EXIT_FAILURE);
    }
    return semid;
}

