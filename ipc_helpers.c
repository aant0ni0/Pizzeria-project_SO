#include "ipc_helpers.h"


int create_msg_queue(key_t){
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Błąd: Nie udało się utworzyć kolejki komunikatów za pomocą msgget. Upewnij się, że używany klucz IPC jest poprawny i że proces ma wystarczające uprawnienia do tworzenia zasobów IPC.");
        exit(EXIT_FAILURE);
    }
    return msgid;

}

void remove_msg_queue(int msgid){
    if(msgctl(msgid, IPC_RMID, NULL) == -1){
        perror("Błąd: Nie udało się usunąć kolejki komunikatów za pomocą msgctl. Upewnij się, że identyfikator kolejki jest poprawny oraz że proces ma wystarczające uprawnienia do jej usunięcia.");
    }
}


int get_msg_queue(key_t key){
    int msgid = msgget(key, 0666);
    if(msgid == -1){
        perror("Błąd: Nie udało się uzyskać identyfikatora kolejki komunikatów za pomocą msgget. Upewnij się, że kolejka została utworzona i proces ma odpowiednie uprawnienia.");
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

int get_semaphore(key_t key){
    int semid = semget(key, 0, 0666);
    if (semid == -1) {
        perror("Błąd: Nie udało się uzyskać dostępu do semafora za pomocą semget. Upewnij się, że semafor z podanym kluczem IPC istnieje oraz że proces ma wystarczające uprawnienia do odczytu i zapisu.");
        exit(EXIT_FAILURE); 
    }
    return semid;

}

void remove_semaphore(int semid){
    if(semctl(semid, 0, IPC_RMID)==-1){
        perror("Błąd: Nie udało się usunąć semafora za pomocą semctl. Upewnij się, że semafor istnieje i że proces ma odpowiednie uprawnienia do jego usunięcia.");
    }
}


void sempahore_down(int semid){
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1) {
        perror("Błąd: Nie udało się obniżyć wartości semafora za pomocą semop. Upewnij się, że semafor istnieje oraz że proces ma wystarczające uprawnienia do operacji na semaforze.");
        exit(EXIT_FAILURE);
    }
    
}


void sempahore_up(int semid){
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = 0;
    if (semop(semid, &op, 1) == -1) {
        perror("Błąd: Nie udało się podnieść wartości semafora za pomocą semop. Upewnij się, że semafor istnieje oraz że proces ma wystarczające uprawnienia do operacji na semaforze.");
        exit(EXIT_FAILURE);
    }

}

