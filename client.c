#include "common.h"
#include "ipc_helpers.h"
#include "sharedmem_helpers.h"

static void fire_exit_handler(int signo) {
    if (signo == FIRE_SIGNAL) {
        printf("[KLIENT %d] Otrzymałem sygnał pożaru. Wychodzę NATYCHMIAST!\n", getpid());
        exit(0);
    }
}


int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(stderr, "Poprawne użycie: %s <liczba_osób_w_grupie> <czas_jedzenia_w_sekundach>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int groupSize = atoi(argv[1]);
    int eatTime = atoi(argv[2]);

    struct sigaction sa;
    sa.sa_handler = fire_exit_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);
    

    if (groupSize < 1 || groupSize > MAX_GROUP_SIZE) {
        fprintf(stderr, "[KLIENT] Niepoprawny rozmiar grupy\n");
        exit(EXIT_FAILURE);
    }

    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if(key == -1){
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int msgid = get_msg_queue(key);

    struct msgbuf_request req;
    req.mtype = 1;
    req.groupSize = groupSize;
    req.pidClient = getpid();

    if(msgsnd(msgid, &req, sizeof(req) - sizeof(long), 0) == -1)   {
        perror("Błąd: Nie udało się wysłać wiadomości do kolejki komunikatów za pomocą msgsnd. Upewnij się, że kolejka istnieje i że proces ma odpowiednie uprawnienia.");
        exit(EXIT_FAILURE);
    }

    struct msgbuf_response resp;
    if(msgrcv(msgid, &resp, sizeof(resp) - sizeof(long), 2 , 0) == -1){
        perror("Błąd: Nie udało się odebrać wiadomości z kolejki komunikatów za pomocą msgrcv. Upewnij się, że kolejka istnieje, jest dostępna oraz że proces ma odpowiednie uprawnienia.");
        exit(EXIT_FAILURE);
    }

    if(!resp.canSit){
        printf("[KLIENT %d] Brak wolnego miejsca. Wychodzę.\n", getpid());
        return 0;
    }
    else{
        printf("[KLIENT %d] Otrzymałem stolik %d-osobowy. Jem przez %d sekund...\n", getpid(), resp.tableSize, eatTime);
        sleep(eatTime);
        printf("[KLIENT %d] Skończyłem, wychodzę.\n", getpid());
        struct msgbuf_release release_msg;
        release_msg.mtype = 3;  
        release_msg.tableSize = resp.tableSize;
        release_msg.groupSize = req.groupSize;  
        release_msg.pidClient = getpid();

        if (msgsnd(msgid, &release_msg, sizeof(release_msg) - sizeof(long), 0) == -1) {
            perror("Błąd: Nie udało się wysłać wiadomości o zwolnieniu stolika.");
            exit(EXIT_FAILURE);
        }
    }
}


