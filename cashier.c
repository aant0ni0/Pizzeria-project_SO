#include "common.h"
#include <ctype.h>

static volatile sig_atomic_t fireHappened 0;

void fire_handler(int signo){
    if(signo == FIRE_SINGAL){
        fireHappened = 1;
    }
}



int main(){
    if(argc < 5){
        fprintf(stderr, "Wprowadź dane w formacie: %s X1 X2 X3 X4\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= 4; i++) {
    char* arg = argv[i];
    for (char* p = arg; *p != '\0'; p++) {
        if (!isdigit(*p)) { 
            fprintf(stderr, "Błąd: Argument %d (%s) musi być liczbą całkowitą większą od 0.\n", i, arg);
            exit(EXIT_FAILURE);
        }
    }
}

    int x1 = atoi(argv[1]);
    int x2 = atoi(argv[2]);
    int x3 = atoi(argv[3]);
    int x4 = atoi(argv[4]);


    //ustawienie obslugi sygnalu
    struct sigaction sa;
    sa.sa_handler() = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SINGAL, &sa, NULL);

    
    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if(key == -1){
        perror("Błąd: Nie udało się utworzyć klucza IPC za pomocą ftok");
        exit(EXIT_FAILURE);
    }


    int msgid = create_msg_queue(key);

    int shmid = create_shared_memory(key);
    TablesState* tables = attach_shared_memory(shmid);

    int semid = create_semaphore(key);

}