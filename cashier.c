#include "common.h"
#include <ctype.h>

static volatile sig_atomic_t fireHappened 0;

void fire_handler(int signo){
    if(signo == FIRE_SINGAL){
        fireHappened = 1;
    }
}

int check_and_seat_group(TablesState* state, int groupSize) {
    switch (groupSize) {
        case 1:
            if (state->free_1_person_tables > 0) {
                state->free_1_person_tables--;
                return 1;
            } else if (state->half_occupied_2_person_tables > 0) {
                state->half_occupied_2_person_tables--;
                return 2;
            } else if (state->free_2_person_tables > 0) {
                state->free_2_person_tables--;
                state->half_occupied_2_person_tables++; 
                return 2;
            } else if (state->free_4_person_tables > 0) {
                state->free_4_person_tables--;
                state->half_occupied_4_person_tables++;
                return 4;
            }
            break;

        case 2:
            if (state->free_2_person_tables > 0) {
                state->free_2_person_tables--;
                return 2;
            } else if (state->half_occupied_4_person_tables > 0) {
                state->half_occupied_4_person_tables--;
                return 4;
            } else if (state->free_4_person_tables > 0) {
                state->free_4_person_tables--;
                state->half_occupied_4_person_tables++;
                return 4;
            }
            break;

        case 3:
            if (state->free_3_person_tables > 0) {
                state->free_3_person_tables--;
                return 3;
            } else if (state->free_4_person_tables > 0) {
                state->free_4_person_tables--;
                return 4;
            }
            break;

        default:
            return 0;
    }
    return 0; 
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


    sempahore_down(semid);
    tables->free_1_person_tables = x1;
    tables->free_2_person_tables = x2;
    tables->free_3_person_tables = x3;
    tables->free_4_person_tables = x4;
    sempahore_up(semid);

    printf("[KASJER] Start pizzerii: stoliki 1-os:%d 2-os:%d 3-os:%d 4-os:%d\n",
           x1, x2, x3, x4);
    

    while (!fireHappened) {
        struct msgbuf_request req;
        ssize_t ret = msgrcv(msgid, &req, sizeof(req) - sizeof(long), 1, IPC_NOWAIT);
        if (ret == -1) {
            if (errno == ENOMSG) {
                usleep(100000);
                continue;
            } else {
                perror("msgrcv");
                break;
            }
        }

        struct msgbuf_response resp;
        resp.mtype = 2; 
        resp.canSit = false;
        resp.tableSize = 0;

        sempahore_down(semid);
        int tableAssigned = check_and_seat_group(tables, req.groupSize);
        sempahore_up(semid);

        if(tableAssigned > 0){
            resp.canSit = true;
            resp.tableSize = tableAssigned;
        }

        if (msgsnd(msgid, &resp, sizeof(resp) - sizeof(long), 0) == -1) {
            perror("Błąd: Nie udało się wysłać odpowiedzi do klienta za pomocą msgsnd. Upewnij się, że kolejka komunikatów istnieje, nie jest pełna oraz że proces ma wystarczające uprawnienia do zapisu.");
        }

    }

    printf("[KASJER] Pożar lub koniec pracy. Zamykam pizzerię.\n");

    detach_shared_memory(tables);
    remove_shared_memory(shmid);
    remove_msg_queue(msgid);
    remove_semaphore(semid);


    return 0;

}