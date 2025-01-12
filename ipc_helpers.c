#include "ipc_helpers.h"


int create_msg_queue(key_t key){
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





