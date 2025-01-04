#include "common.h"
#include "ipc_helpers.h"
#include "sharedmem_helpers.h"

int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(stderr, "Poprawne użycie: %s <liczba_osób_w_grupie> <czas_jedzenia_w_sekundach>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int groupSize = atoi(argv[1]);
    int eatTime = atoi(argv[2]);

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
    }
}


