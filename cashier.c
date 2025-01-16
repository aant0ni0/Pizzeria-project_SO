#include "common.h"
#include "ipc_helpers.h"
#define MAX_CLIENTS 100

struct timespec req = {0, 100 * 1000000L};


// flaga pożaru
static volatile sig_atomic_t fireHappened = 0;

// tablica PID-ów (kasjer może mieć 1+ klientów)
static pid_t clientPIDs[MAX_CLIENTS];
static int clientCount = 0;

// GLOBALNE TABLICE: tableCapacity[i], tableState[i]
static int* tableCapacity = NULL; 
static int* tableState   = NULL; 
static int  totalTables  = 0;    // łączna liczba stolików

static int x1 = 0;
static int x2 = 0;
static int x3 = 0;
static int x4 = 0;

// Handler sygnału
void fire_handler(int signo) {
    if (signo == FIRE_SIGNAL) {
        fireHappened = 1;
    }
}

void sleep_for_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;             
    ts.tv_nsec = (milliseconds % 1000) * 1000000; 
    nanosleep(&ts, NULL);
}

int is_positive_integer(const char *str) {
    if (str == NULL || *str == '\0') return 0; // Pusty ciąg lub NULL
    while (*str) {
        if (!isdigit(*str)) return 0; // Jeśli znak nie jest cyfrą, zwróć 0 (błąd)
        str++;
    }
    return 1; // Wszystkie znaki są cyframi
}


void init_tables(int x1, int x2, int x3, int x4) {
    totalTables = x1 + x2 + x3 + x4;
    tableCapacity = malloc(sizeof(int) * totalTables);
    tableState   = malloc(sizeof(int) * totalTables);

    int idx = 0;
    // 1-os.
    for (int i = 0; i < x1; i++) {
        tableCapacity[idx] = 1;
        tableState[idx]    = 2; // 2 = w pełni wolny
        idx++;
    }
    // 2-os.
    for (int i = 0; i < x2; i++) {
        tableCapacity[idx] = 2;
        tableState[idx]    = 2;
        idx++;
    }
    // 3-os.
    for (int i = 0; i < x3; i++) {
        tableCapacity[idx] = 3;
        tableState[idx]    = 2;
        idx++;
    }
    // 4-os.
    for (int i = 0; i < x4; i++) {
        tableCapacity[idx] = 4;
        tableState[idx]    = 2;
        idx++;
    }
}

int check_and_seat_group(int groupSize) {
    switch(groupSize){
        case 1:
            for(int i = 0; i < x1; i++){
                if(tableState[i] > 0){
                    tableState[i] = 0;
                    return i;
                } 
            }
            for(int i = x1; i < x1 + x2; i++){
                if(tableState[i] > 0){
                    tableState[i] = tableState[i] - 1;
                    return i;
                } 
            }
            break;
            

        case 2:
            for(int i = x1; i < x1 + x2 ; i++){
                if(tableState[i] == 2){
                    tableState[i] = 0;
                    return i;
                }
            }
            for(int i = x1 + x2 + x3; i < x1 + x2 + x3 + x4; i++){
                if(tableState[i] > 0){
                    tableState[i] = tableState[i] - 1;
                    return i;
                }
            }
            break;

        case 3:
            for(int i = x1 + x2; i < x1 + x2 + x3; i++){
                if(tableState[i] > 0){
                    tableState[i] = 0;
                    return i;
                } 
            }
            for(int i = x1 + x2 + x3; i < x1 + x2 + x3 + x4; i++){
                if(tableState[i] == 2){
                    tableState[i] = 0;
                    return i;
                } 
            }
            break;
    }
    return -1;
    
}

void free_table(int tableIndex, int groupSize) {
    int cap = tableCapacity[tableIndex];

    if (cap / groupSize == 2) {
        tableState[tableIndex] = tableState[tableIndex] + 1;
    }
    else if (groupSize == cap || (groupSize == 3 && cap == 4)) {
        tableState[tableIndex] = 2;
    }
}


int main(int argc, char* argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Błąd: Niewystarczająca liczba argumentów.\n");
        fprintf(stderr, "Podaj dane w formacie: %s <liczba stolików 1-osobowych> <liczba stolików 2-osobowych> <liczba stolików 3-osobowych> <liczba stolików 4-osobowych>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        if (!is_positive_integer(argv[i])) {
            fprintf(stderr, "Błąd: Argumenty muszą być dodatnimi liczbami całkowitymi.\n");
            exit(EXIT_FAILURE);
        }
    }

  

    // Obsługa sygnału
    struct sigaction sa;
    sa.sa_handler = fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);

    // Inicjalizacja tablic stolików
    x1 = atoi(argv[1]);
    x2 = atoi(argv[2]);
    x3 = atoi(argv[3]);
    x4 = atoi(argv[4]);

    if ((x1 + x2 + x3 + x4) > MAX_TABLES) {
        fprintf(stderr, "Łączna liczba stolików nie może przekroczyć %d.\n", MAX_TABLES);
        exit(EXIT_FAILURE);
    }

    init_tables(x1, x2, x3, x4);

    printf("[KASJER] Start pizzerii: 1-os:%d, 2-os:%d, 3-os:%d, 4-os:%d (razem %d stolików)\n\n",
           x1, x2, x3, x4, (x1 + x2 + x3 + x4));

    // Kolejka
    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if (key == -1) {
        perror("[KASJER] Błąd funkcji ftok - nie można wygenerować klucza IPC.");
        exit(EXIT_FAILURE);
    }

    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("[KASJER] Błąd funkcji msgget - nie można utworzyć kolejki komunikatów.");
        exit(EXIT_FAILURE);
    }

    // Pętla obsługi dopóki brak pożaru
    while (!fireHappened) {

        // 1) Odbierz zapytanie (mtype=1)
        {
            struct msgbuf_request req;
            ssize_t ret = msgrcv(msgid, &req, sizeof(req) - sizeof(long), 1, IPC_NOWAIT);
            if (ret != -1) {

                // Rejestrujemy PID
                if (clientCount < MAX_CLIENTS) {
                    clientPIDs[clientCount++] = req.pidClient;
                }

                // Przygotowujemy odpowiedź
                struct msgbuf_response resp;
                resp.mtype = 2;
                resp.canSit = false;
                resp.tableIndex = -1;
                resp.tableSize   = 0;
                resp.groupId    = req.groupId;

                int idx = check_and_seat_group(req.groupSize);
                

                if (idx >= 0) {
                    resp.canSit = true;
                    resp.tableIndex = idx;
                    resp.tableSize = tableCapacity[idx];
                }

               if (msgsnd(msgid, &resp, sizeof(resp) - sizeof(long), IPC_NOWAIT) == -1) {
                if (errno == EAGAIN) {
                    fprintf(stderr, "\n\n[KASJER] Błąd: Kolejka komunikatów jest pełna. Nie można wysłać odpowiedzi dla grupy %d (%d-os.)\n\n",
                    req.groupId, req.groupSize);
                    break;
                } else {
                    perror("[KASJER] Błąd: Nie udało się wysłać odpowiedzi za pomocą msgsnd");
                }
                } else if (!resp.canSit) {
                    printf("\n[KASJER] Grupa #%d (%d-os.) NIE otrzymała stolika.\n",
                    resp.groupId, req.groupSize);
                }

                } else {
                    if (errno != ENOMSG && errno != EINTR) {
                      perror("[KASJER] Błąd: Nie udało się odebrać zapytania za pomocą msgrcv (mtype=1)");
                    }
                }
        }

        // 2) Odbierz zwolnienie (mtype=3)
        {
            struct msgbuf_release rel;
            ssize_t ret = msgrcv(msgid, &rel, sizeof(rel) - sizeof(long), 3, IPC_NOWAIT);
            if (ret != -1) {

                free_table(rel.tableIndex, rel.groupSize);

                printf("\n[KASJER] Zwolniono stolik %d os. (grupa %d os.)\n\n",
                       tableCapacity[rel.tableIndex], rel.groupSize);
            } else {
                if (errno != ENOMSG && errno != EINTR) {
                    perror("[KASJER] Błąd: Nie udało się odebrać zwolnienia stolika za pomocą msgrcv (mtype=3)");
                }
            }
        }
    }

    // Pożar
    if(fireHappened){
        printf("[KASJER] Pożar wykryty -> wypraszam klientów.\n");
    }
    else{
        printf("\n[KASJER] Czas zamykać (kolejka komunikatów pełna) -> wypraszam klientów.\n");
    }
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
            sleep_for_ms(100);
        }
    }

    // Usuwamy kolejkę
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("[KASJER] Błąd: Nie udało się usunąć kolejki komunikatów za pomocą msgctl");
    }


    free(tableCapacity);
    free(tableState);

    printf("[KASJER] Wszyscy klienci wyszli. Kończę.\n");
    return 0;
}