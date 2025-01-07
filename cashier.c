#include "common.h"
#include <ctype.h>
#include "ipc_helpers.h"
#include "sharedmem_helpers.h"
#include <ctype.h>     
#include <unistd.h>   

#define MAX_CLIENTS 100

static volatile sig_atomic_t fireHappened = 0;

static pid_t clientPIDs[MAX_CLIENTS];
static int clientCount = 0;

void fire_handler(int signo){
    if(signo == FIRE_SIGNAL){
        fireHappened = 1;
    }
}

//ZAKLADAMY ZE GDY ISTNIEJĄ DWA TAK SAMO DUZE, W POLOWIE WOLNE STOLIKI, SIEDZACY PRZY NICH LUDZIE PRZYSIADAJA SIE DO SIEBIE ZEBY ZWOLNIC JEDEN PEŁNY STOLIK - TAKI JEST USTALONY REGULAMIN PIZZERI 
// (Rozwiązanie jest bardziej dochodowe dla wlasciciela a tresc zadania go nie wyklucza)
void free_table(TablesState* state, int tableSize, int groupSize) {
    switch(tableSize) {
        case 1:
            state->free_1_person_tables++;
            break;

        case 2:
            if (groupSize == 2) {
                state->free_2_person_tables++;
            } else if (groupSize == 1 && state->half_occupied_2_person_tables > 0) {
                state->half_occupied_2_person_tables--;
                state->free_2_person_tables++;
            } else if(groupSize ==1 && state->half_occupied_2_person_tables == 0 ){
                state->half_occupied_2_person_tables++;
            }
            break;

        case 3:

            state->free_3_person_tables++;
            break;

        case 4:
            if (groupSize == 4) {
                state->free_4_person_tables++;
            }  else if (groupSize == 2 && state->half_occupied_4_person_tables > 0) {
                state->half_occupied_4_person_tables--;
                state->free_4_person_tables++;
            } else if(groupSize == 2 && state->half_occupied_4_person_tables == 0 ){
                state->half_occupied_4_person_tables++;
            }
            break;
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

int main(int argc, char* argv[]){
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
    sa.sa_handler = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);

    
    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if(key == -1){
        perror("Błąd: Nie udało się utworzyć klucza IPC za pomocą ftok");
        exit(EXIT_FAILURE);
    }


    int msgid = create_msg_queue(key);

    int shmid = create_shared_memory(key);
    TablesState* tables = attach_shared_memory(shmid);

    int semid = create_semaphore(key);


    semaphore_down(semid);
    tables->free_1_person_tables = x1;
    tables->free_2_person_tables = x2;
    tables->free_3_person_tables = x3;
    tables->free_4_person_tables = x4;

    tables->half_occupied_2_person_tables = 0;
    tables->half_occupied_4_person_tables = 0;
    semaphore_up(semid);

    printf("[KASJER] Start pizzerii: stoliki 1-os:%d 2-os:%d 3-os:%d 4-os:%d\n",
           x1, x2, x3, x4);
    

    while (!fireHappened) {
        bool noMessage = true;

        // 1) Spróbuj odebrać zapytania o stolik (mtype=1)
        {
            struct msgbuf_request req;
            ssize_t ret = msgrcv(msgid, &req, sizeof(req) - sizeof(long), 1, IPC_NOWAIT);
            if (ret != -1) {
                noMessage = false;
                if (clientCount < MAX_CLIENTS) {
                    clientPIDs[clientCount++] = req.pidClient;
                }

                struct msgbuf_response resp;
                resp.mtype = 2;
                resp.canSit = false;
                resp.tableSize = 0;

                semaphore_down(semid);
                int tableAssigned = check_and_seat_group(tables, req.groupSize);
                semaphore_up(semid);

                if (tableAssigned > 0) {
                    resp.canSit = true;
                    resp.tableSize = tableAssigned;
                }

                if (msgsnd(msgid, &resp, sizeof(resp) - sizeof(long), 0) == -1) {
                    perror("[KASJER] Błąd msgsnd w odpowiedzi");
                }
            }
            else {
                if (errno != ENOMSG && errno != EINTR) {
                    perror("[KASJER] Błąd msgrcv (mtype=1)");
                }
            }
        }

        // 2) Spróbuj odebrać komunikaty o zwolnieniu stolika (mtype=3)
        {
            struct msgbuf_release rel;
            ssize_t ret = msgrcv(msgid, &rel, sizeof(rel) - sizeof(long), 3, IPC_NOWAIT);
            if (ret != -1) {
                noMessage = false;

                semaphore_down(semid);
                free_table(tables, rel.tableSize, rel.groupSize);
                semaphore_up(semid);

                printf("[KASJER] Klient %d zwolnił stolik %d-os (grupa:%d)\n",
                        rel.pidClient, rel.tableSize, rel.groupSize);
            }
            else {
                if (errno != ENOMSG && errno != EINTR) {
                    perror("[KASJER] Błąd msgrcv (mtype=3)");
                }
            }
        }

        // Jeśli nie było żadnej wiadomości do obsłużenia, zrób mały sleep
        if (noMessage) {
            usleep(100000);
        }
    }



    printf("[KASJER] Pożar wykryty. Wypraszam klientów...\n");

    for (int i = 0; i < clientCount; i++) {
        kill(clientPIDs[i], FIRE_SIGNAL);  
    }

    bool someClientAlive = true;
    while (someClientAlive) {
        someClientAlive = false;
        for (int i = 0; i < clientCount; i++) {
            if (clientPIDs[i] == 0) {
                continue; 
            }
            if (kill(clientPIDs[i], 0) == 0) {
                someClientAlive = true;
            } else {
                if (errno == ESRCH) {
                    clientPIDs[i] = 0;
                }
            }
        }
        if (someClientAlive) {
            usleep(200000);
        }
    }

    printf("[KASJER] Wszyscy klienci wyszli. Zamykam pizzerię.\n");

    detach_shared_memory(tables);
    remove_shared_memory(shmid);
    remove_msg_queue(msgid);
    remove_semaphore(semid);


    return 0;

}