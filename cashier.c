#include "common.h"

// Flaga wykrycia pożaru (volatile, bo zmienna może być zmieniana w handlerze sygnału)
static volatile sig_atomic_t fireHappened = 0;

// Tablica PID-ów grup klientów, maksymalnie 100 grup
static pid_t clientPIDs[MAX_CLIENTS];
static int clientCount = 0; // Liczba aktualnych klientów

// PID spawnera, ustawiany po otrzymaniu rejestracji (mtype=10)
static pid_t spawnerPid = 0;

// Tablice przechowujące informacje o stolikach
static int* tableCapacity = NULL; // Pojemności stolików
static int* tableState    = NULL; // Stany stolików (0 - zajęty, >0 - wolny)
static int  totalTables   = 0;    // Łączna liczba stolików

// Liczba stolików w zależności od rozmiaru
static int x1 = 0; // 1-osobowe
static int x2 = 0; // 2-osobowe
static int x3 = 0; // 3-osobowe
static int x4 = 0; // 4-osobowe

// Handler sygnału pożaru
void fire_handler(int signo) {
    if (signo == FIRE_SIGNAL) {
        fireHappened = 1; // Ustawiamy flagę, aby zakończyć działanie pętli
    }
}

// Pomocnicze usypianie (używane opcjonalnie w pętli głównej)
void sleep_for_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;          // Sekundy
    ts.tv_nsec = (milliseconds % 1000) * 1000000; // Nanosekundy
    nanosleep(&ts, NULL);
}

// Sprawdzenie, czy podany ciąg znaków to dodatnia liczba całkowita
int is_positive_integer(const char *str) {
    if (str == NULL || *str == '\0') return 0;
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

// Inicjalizacja stolików w zależności od liczby przekazanych parametrów
void init_tables(int x1, int x2, int x3, int x4) {
    totalTables = x1 + x2 + x3 + x4; // Łączna liczba stolików
    tableCapacity = malloc(sizeof(int) * totalTables);
    tableState    = malloc(sizeof(int) * totalTables);

    int idx = 0;
    // Inicjalizacja stolików 1-osobowych
    for (int i = 0; i < x1; i++) {
        tableCapacity[idx] = 1;
        tableState[idx]    = 2; // Pełna dostępność (2 miejsca wolne)
        idx++;
    }
    // Inicjalizacja stolików 2-osobowych
    for (int i = 0; i < x2; i++) {
        tableCapacity[idx] = 2;
        tableState[idx]    = 2;
        idx++;
    }
    // Inicjalizacja stolików 3-osobowych
    for (int i = 0; i < x3; i++) {
        tableCapacity[idx] = 3;
        tableState[idx]    = 2;
        idx++;
    }
    // Inicjalizacja stolików 4-osobowych
    for (int i = 0; i < x4; i++) {
        tableCapacity[idx] = 4;
        tableState[idx]    = 2;
        idx++;
    }
}

// Logika przydzielania grup do stolików
int check_and_seat_group(int groupSize) {
    if (groupSize == 1) { // Dla grup 1-osobowych
        for (int i = 0; i < x1; i++) {
            if (tableState[i] > 0) {
                tableState[i] = 0;
                return i; // Zwracamy indeks stolika
            }
        }
        for (int i = x1; i < x1 + x2; i++) {
            if (tableState[i] > 0) {
                tableState[i]--;
                return i;
            }
        }
    }
    else if (groupSize == 2) { // Dla grup 2-osobowych
        for (int i = x1; i < x1 + x2; i++) {
            if (tableState[i] == 2) {
                tableState[i] = 0;
                return i;
            }
        }
        for (int i = x1 + x2 + x3; i < totalTables; i++) {
            if (tableState[i] > 0) {
                tableState[i]--;
                return i;
            }
        }
    }
    else if (groupSize == 3) { // Dla grup 3-osobowych
        for (int i = x1 + x2; i < x1 + x2 + x3; i++) {
            if (tableState[i] > 0) {
                tableState[i] = 0;
                return i;
            }
        }
        for (int i = x1 + x2 + x3; i < totalTables; i++) {
            if (tableState[i] == 2) {
                tableState[i] = 0;
                return i;
            }
        }
    }
    return -1; // Brak dostępnych stolików
}

// Logika zwalniania stolika po zakończeniu jedzenia
void free_table(int tableIndex, int groupSize) {
    int cap = tableCapacity[tableIndex];
    if (cap / groupSize == 2) { // Przywrócenie dostępności częściowej
        tableState[tableIndex]++;
    } else {
        tableState[tableIndex] = 2; // Stoliki w pełni wolne
    }
}

int main(int argc, char* argv[]) {
    // Sprawdzenie liczby argumentów i ich poprawności
    if (argc < 5) {
        fprintf(stderr, "Użyj: %s <1-os.> <2-os.> <3-os.> <4-os.>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    for (int i = 1; i <= 4; i++) {
        if (!is_positive_integer(argv[i])) {
            fprintf(stderr, "Błąd: Argumenty muszą być dodatnimi liczbami całkowitymi.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Rejestracja handlera sygnału pożaru
    struct sigaction sa;
    sa.sa_handler = fire_handler; // Ustawiamy funkcję obsługi sygnału
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);

    // Pobranie parametrów (liczby stolików różnej wielkości)
    x1 = atoi(argv[1]);
    x2 = atoi(argv[2]);
    x3 = atoi(argv[3]);
    x4 = atoi(argv[4]);

    // Inicjalizacja stolików
    init_tables(x1, x2, x3, x4);

    // Informacja o otwarciu pizzerii
    printf("[KASJER PID=%d] Otwieram pizzerię: 1-os:%d, 2-os:%d, 3-os:%d, 4-os:%d\n",
           getpid(), x1, x2, x3, x4);

    // Tworzenie unikalnego klucza IPC
    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if (key == -1) {
        perror("[KASJER] Błąd ftok");
        exit(EXIT_FAILURE);
    }

    // Tworzenie kolejki komunikatów
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("[KASJER] Błąd msgget");
        exit(EXIT_FAILURE);
    }

    // Oczekiwanie na proces spawner rejestrujący się w kolejce
    printf("[KASJER] Oczekuję na klientów (spawner)...\n");
    struct msgbuf_spawnerReg reg;
    if (msgrcv(msgid, &reg, sizeof(reg) - sizeof(long), 10, 0) == -1) {
        perror("[KASJER] Błąd msgrcv (spawnerReg)");
        exit(EXIT_FAILURE);
    }
    spawnerPid = reg.spawnerPid; // Rejestracja PID spawnera

    // Pętla główna obsługująca klientów i zwalnianie stolików
    while (!fireHappened) {
        // Odbieranie zapytań od klientów (mtype=1)
        struct msgbuf_request req;
        if (msgrcv(msgid, &req, sizeof(req) - sizeof(long), 1, IPC_NOWAIT) != -1) {
            // Rejestracja klienta
            if (clientCount < MAX_CLIENTS) {
                clientPIDs[clientCount++] = req.pidClient;
            }

            // Przygotowanie odpowiedzi
            struct msgbuf_response resp;
            resp.mtype = 2;
            resp.canSit = false;
            resp.tableIndex = -1;
            resp.tableSize = 0;
            resp.groupId = req.groupId;

            // Próba przydzielenia stolika
            int idx = check_and_seat_group(req.groupSize);
            if (idx >= 0) {
                resp.canSit = true;
                resp.tableIndex = idx;
                resp.tableSize = tableCapacity[idx];
            }

            // Wysyłanie odpowiedzi
            if (msgsnd(msgid, &resp, sizeof(resp) - sizeof(long), 0) == -1) {
                perror("[KASJER] Błąd msgsnd (odpowiedź)");
            }
        }

        // Odbieranie zwolnień stolików (mtype=3)
        struct msgbuf_release rel;
        if (msgrcv(msgid, &rel, sizeof(rel) - sizeof(long), 3, IPC_NOWAIT) != -1) {
            free_table(rel.tableIndex, rel.groupSize); // Zwolnienie stolika
            printf("\n\n[KASJER] Zwolniono stolik %d-os. (grupa %d-os.)\n",
                   tableCapacity[rel.tableIndex], rel.groupSize);
        }
    }

    // Wykrycie sygnału pożaru, kończenie pracy
    if (spawnerPid > 0) {
        kill(spawnerPid, FIRE_SIGNAL); // Informowanie spawnera
    }

    // Wysyłanie sygnału do wszystkich klientów
    for (int i = 0; i < clientCount; i++) {
        if (clientPIDs[i] != 0) {
            kill(clientPIDs[i], FIRE_SIGNAL);
        }
    }

    // Oczekiwanie na zakończenie pracy klientów
    bool someClientAlive = true;
    while (someClientAlive) {
        someClientAlive = false;
        for (int i = 0; i < clientCount; i++) {
            if (clientPIDs[i] != 0 && kill(clientPIDs[i], 0) == 0) {
                someClientAlive = true;
            } else {
                clientPIDs[i] = 0;
            }
        }
        if (someClientAlive) {
            sleep_for_ms(100);
        }
    }

    // Usuwanie kolejki komunikatów
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("[KASJER] Błąd msgctl (usuwanie kolejki)");
    }

    // Zwolnienie pamięci zajmowanej przez tablice stolików
    free(tableCapacity);
    free(tableState);

    // Zakończenie pracy kasjera
    printf("[KASJER] Wszyscy klienci wyszli. Kończę.\n");
    return 0;
}

