#include "common.h"

#define MAX_CLIENTS 100

static volatile sig_atomic_t fireHappened = 0;

static pid_t clientPIDs[MAX_CLIENTS];
static int clientCount = 0;

static TablesState tables;


void fire_handler(int signo) {
    if (signo == FIRE_SIGNAL) {
        fireHappened = 1;
    }
}


int check_and_seat_group(int groupSize) {
    switch (groupSize) {
        case 1:
            if (tables.free_1_person_tables > 0) {
                tables.free_1_person_tables--;
                return 1;
            } else if (tables.half_occupied_2_person_tables > 0) {
                tables.half_occupied_2_person_tables--;
                return 2;
            } else if (tables.free_2_person_tables > 0) {
                tables.free_2_person_tables--;
                tables.half_occupied_2_person_tables++;
                return 2;
            } else if (tables.free_4_person_tables > 0) {
                tables.free_4_person_tables--;
                tables.half_occupied_4_person_tables++;
                return 4;
            }
            break;

        case 2:
            if (tables.free_2_person_tables > 0) {
                tables.free_2_person_tables--;
                return 2;
            } else if (tables.half_occupied_4_person_tables > 0) {
                tables.half_occupied_4_person_tables--;
                return 4;
            } else if (tables.free_4_person_tables > 0) {
                tables.free_4_person_tables--;
                tables.half_occupied_4_person_tables++;
                return 4;
            }
            break;

        case 3:
            if (tables.free_3_person_tables > 0) {
                tables.free_3_person_tables--;
                return 3;
            } else if (tables.free_4_person_tables > 0) {
                tables.free_4_person_tables--;
                return 4;
            }
            break;
    }
    return 0; 
}


void free_table(int tableSize, int groupSize) {
    switch (tableSize) {
        case 1:
            tables.free_1_person_tables++;
            break;
        case 2:
            if (groupSize == 2) {
                tables.free_2_person_tables++;
            } else {
                tables.free_2_person_tables++;
            }
            break;
        case 3:
            tables.free_3_person_tables++;
            break;
        case 4:
            tables.free_4_person_tables++;
            break;
    }
}


int main(int argc, char* argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Użycie: %s X1 X2 X3 X4\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);

    tables.free_1_person_tables = atoi(argv[1]);
    tables.free_2_person_tables = atoi(argv[2]);
    tables.free_3_person_tables = atoi(argv[3]);
    tables.free_4_person_tables = atoi(argv[4]);
    tables.half_occupied_2_person_tables = 0;
    tables.half_occupied_4_person_tables = 0;

    printf("[KASJER] Start pizzerii: 1-os:%d, 2-os:%d, 3-os:%d, 4-os:%d\n",
           tables.free_1_person_tables,
           tables.free_2_person_tables,
           tables.free_3_person_tables,
           tables.free_4_person_tables);


    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    while (!fireHappened) {
        bool noMessage = true;

        // 1) Odbierz zapytanie o stolik (mtype=1)
        {
            struct msgbuf_request req;
            ssize_t ret = msgrcv(msgid, &req, sizeof(req) - sizeof(long), 1, IPC_NOWAIT);
            if (ret != -1) {
                noMessage = false;

                // Zapisz PID w tablicy (by móc wysłać sygnał w razie pożaru)
                if (clientCount < MAX_CLIENTS) {
                    clientPIDs[clientCount++] = req.pidClient;
                }

                // Przygotuj odpowiedź
                struct msgbuf_response resp;
                resp.mtype     = 2;
                resp.canSit    = false;
                resp.tableSize = 0;
                resp.groupId   = req.groupId;  // odsyłamy, by klient wiedział

                int table = check_and_seat_group(req.groupSize);
                if (table > 0) {
                    resp.canSit    = true;
                    resp.tableSize = table;
                }

                if (msgsnd(msgid, &resp, sizeof(resp) - sizeof(long), 0) == -1) {
                    perror("[KASJER] msgsnd (response)");
                }
            }
            else {
                // msgrcv zwróci -1
                if (errno != ENOMSG && errno != EINTR) {
                    // EINVAL / EIDRM => queue removed or invalid
                    perror("[KASJER] Błąd msgrcv (mtype=1)");
                }
            }
        }

        // 2) Odbierz zwolnienie stolika (mtype=3)
        {
            struct msgbuf_release rel;
            ssize_t ret = msgrcv(msgid, &rel, sizeof(rel) - sizeof(long), 3, IPC_NOWAIT);
            if (ret != -1) {
                noMessage = false;

                free_table(rel.tableSize, rel.groupSize);
                printf("[KASJER] Grupa #%d (PID=%d) zwolniła stolik %d-os (groupSize:%d)\n\n",
                       rel.groupId, rel.pidClient, rel.tableSize, rel.groupSize);
            }
            else {
                if (errno != ENOMSG && errno != EINTR) {
                    perror("[KASJER] Błąd msgrcv (mtype=3)");
                }
            }
        }

        // jeżeli nie było żadnego komunikatu, mały sleep
        if (noMessage) {
            usleep(100000);
        }
    }

    // POŻAR
    printf("[KASJER] Pożar wykryty -> wypraszam klientów.\n");
    // Wysyłamy sygnał do wszystkich klientPIDs
    for (int i = 0; i < clientCount; i++) {
        if (clientPIDs[i] != 0) {
            kill(clientPIDs[i], FIRE_SIGNAL);
        }
    }

    // Czekamy, aż klienci się zakończą
    bool someClientAlive = true;
    while (someClientAlive) {
        someClientAlive = false;
        for (int i = 0; i < clientCount; i++) {
            if (clientPIDs[i] == 0) continue;
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

    // Usuwamy kolejkę i kończymy
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("[KASJER] msgctl remove");
    }
    printf("[KASJER] Wszyscy klienci wyszli. Kończę.\n");
    return 0;
}
