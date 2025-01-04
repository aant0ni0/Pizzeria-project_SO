#include "sharedmem_helpers.h"

int create_shared_memory(key_t key){
    int shmid = shmget(key, sizeof(TablesState), IPC_CREAT | 0666);
    if(shmid == -1){
        perror("Błąd: Nie udało się utworzyć segmentu pamięci dzielonej za pomocą shmget. Sprawdź, czy istnieje odpowiedni klucz IPC oraz czy masz wystarczające uprawnienia.");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

TablesState* attach_shared_memory(int shmid){
    void* addr = shmat(shmid, NULL, 0);
    if(addr == (void*)-1){
        perror("Błąd: Nie udało się podłączyć segmentu pamięci dzielonej za pomocą shmat. Upewnij się, że segment pamięci dzielonej został poprawnie utworzony i że proces ma odpowiednie uprawnienia.");
        exit(EXIT_FAILURE);
    }
    return (TablesState*)addr;
}



void detach_shared_memory(const void* shmaddr){
    if(shmdt(shmaddr) == -1){
        perror("Błąd: Nie udało się odłączyć segmentu pamięci dzielonej za pomocą shmdt...");

    }
}

void remove_shared_memory(int shmid){
    if(shmctl(shmid, IPC_RMID, NULL) == -1){
        perror("Błąd: Nie udało się usunąć segmentu pamięci dzielonej za pomocą shmctl. Upewnij się, że identyfikator segmentu jest poprawny oraz że proces ma wystarczające uprawnienia.");
    }

}
