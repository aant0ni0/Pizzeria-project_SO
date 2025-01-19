#include "common.h"
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>

/* =====================================================================
   1) Obsługa sygnału pożaru dla spawnera
   ===================================================================== */
static void spawner_fire_handler(int signo) {
    if (signo == FIRE_SIGNAL) {
        // Sygnał pożaru - kończymy proces spawnera
        exit(0);
    }
}

/* =====================================================================
   2) KOD GRUPY KLIENTÓW (lider + członkowie)
   ===================================================================== */
typedef struct {
    int  groupId;         // ID grupy
    int  groupSize;       // Rozmiar grupy
    int  tableSize;       // Rozmiar przydzielonego stolika
    bool canSit;          // Czy grupa może zająć stolik
    bool responseReady;   // Czy odpowiedź od kasjera jest gotowa
    int  eatTime;         // Czas spędzony przy stoliku
    pthread_mutex_t lock; // Mutex do synchronizacji wątków w grupie
    pthread_cond_t  cond; // Warunek do komunikacji między wątkami
} GroupInfo;

// Handler dla grupy w przypadku sygnału pożaru
static void client_fire_handler(int signo) {
    if (signo == FIRE_SIGNAL) {
        printf("[GRUPA PID=%d] Pożar -> opuszczamy pizzerię\n", getpid());
        exit(0);
    }
}

// Funkcja wątku członka grupy
void* client_thread_func(void* arg) {
    GroupInfo* g = (GroupInfo*)arg;

    // Oczekiwanie na odpowiedź od lidera
    pthread_mutex_lock(&g->lock);
    while (!g->responseReady) {
        pthread_cond_wait(&g->cond, &g->lock);
    }
    bool localCanSit = g->canSit;
    int  localTable  = g->tableSize;
    int  localGId    = g->groupId;
    int  localEat    = g->eatTime;
    pthread_mutex_unlock(&g->lock);

    // Jeśli grupa nie otrzymała stolika, wątek kończy pracę
    if (!localCanSit) {
        pthread_exit(NULL);
    } else {
        // Symulacja jedzenia
        printf("[CZŁONEK GRUPY %d | PID=%d] Jem przy stoliku %d-os. przez %d s\n",
               localGId, (int)getpid(), localTable, localEat);
        sleep(localEat); // Symulacja czasu jedzenia
        pthread_exit(NULL);
    }
}

// Funkcja wątku lidera grupy
void* leader_thread_func(void* arg) {
    GroupInfo* g = (GroupInfo*)arg;

    // Otwieranie kolejki komunikatów
    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if (key == -1) {
        perror("[LEADER] Błąd ftok");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("[LEADER] Błąd msgget (czy kasjer działa?)");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    // Wysłanie zapytania do kasjera
    struct msgbuf_request req;
    req.mtype     = 1;            // Typ wiadomości: zapytanie o stolik
    req.groupId   = g->groupId;
    req.groupSize = g->groupSize;
    req.pidClient = getpid();

    if (msgsnd(msgid, &req, sizeof(req) - sizeof(long), 0) == -1) {
        perror("[LEADER] Błąd msgsnd (zapytanie o stolik)");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    // Oczekiwanie na odpowiedź od kasjera
    struct msgbuf_response resp;
    if (msgrcv(msgid, &resp, sizeof(resp) - sizeof(long), 2, 0) == -1) {
        perror("[LEADER] Błąd msgrcv (odpowiedź kasjera)");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    // Przetwarzanie odpowiedzi
    pthread_mutex_lock(&g->lock);
    g->canSit      = resp.canSit;
    g->tableSize   = resp.tableSize;
    g->responseReady = true;

    // Ustawienie czasu jedzenia (jeśli grupa otrzymała stolik)
    if (resp.canSit) {
        g->eatTime = 7 + (rand() % 6); // 7-12 sekund
    } else {
        g->eatTime = 0;
    }
    pthread_cond_broadcast(&g->cond);
    pthread_mutex_unlock(&g->lock);

    // Jeśli grupa nie otrzymała stolika, lider kończy pracę
    if (!resp.canSit) {
        printf("[LEADER GRUPA #%d] (grupa %d-os.) NIE otrzymała stolika.\n",
           g->groupId, g->groupSize);
        pthread_exit(NULL);
    } else {
        // Symulacja jedzenia przez lidera
        printf("[LIDER GRUPY %d | PID=%d] Jem przy stoliku %d-os. przez %d s\n",
               g->groupId, getpid(), resp.tableSize, g->eatTime);
        sleep(g->eatTime);

        // Po zakończeniu jedzenia lider zwalnia stolik
        struct msgbuf_release rel;
        rel.mtype      = 3; // Typ wiadomości: zwolnienie stolika
        rel.groupId    = g->groupId;
        rel.tableSize  = resp.tableSize;
        rel.tableIndex = resp.tableIndex;
        rel.groupSize  = g->groupSize;
        rel.pidClient  = getpid();

        if (msgsnd(msgid, &rel, sizeof(rel) - sizeof(long), 0) == -1) {
            perror("[LEADER] Błąd msgsnd (zwalnianie stolika)");
        }

        pthread_exit(NULL);
    }
}


// Funkcja obsługująca proces grupy
void group_main_process(int groupId, int groupSize) {
    // Obsługa sygnału pożaru
    struct sigaction sa;
    sa.sa_handler = client_fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);

    srand(time(NULL) ^ (getpid() << 16));

    // Inicjalizacja grupy
    GroupInfo* g = malloc(sizeof(GroupInfo));
    g->groupId       = groupId;
    g->groupSize     = groupSize;
    g->tableSize     = 0;
    g->canSit        = false;
    g->responseReady = false;
    g->eatTime       = 0;

    pthread_mutex_init(&g->lock, NULL);
    pthread_cond_init(&g->cond, NULL);

    pthread_t* tids = calloc(groupSize, sizeof(pthread_t));

    // Uruchomienie wątków grupy
    pthread_create(&tids[0], NULL, leader_thread_func, g); // Lider
    for (int i = 1; i < groupSize; i++) {
        pthread_create(&tids[i], NULL, client_thread_func, g); // Członkowie
    }

    // Oczekiwanie na zakończenie wątków
    for (int i = 0; i < groupSize; i++) {
        pthread_join(tids[i], NULL);
    }
    free(tids);

    // Sprzątanie
    pthread_mutex_destroy(&g->lock);
    pthread_cond_destroy(&g->cond);
    free(g);

    exit(0);
}

/* =====================================================================
   3) MAIN = SPAWNER PROCESÓW-GRUP
   ===================================================================== */
int main(int argc, char* argv[]) {
    // Ignorowanie sygnału SIGCHLD (aby uniknąć procesów zombie)
    struct sigaction sact;
    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = SIG_IGN;
    sact.sa_flags   = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sact, NULL);

    // Obsługa sygnału pożaru
    struct sigaction sa;
    sa.sa_handler = spawner_fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);

    srand(time(NULL));
    printf("Klienci zaczynają przychodzić...\n");

    // Otwieranie kolejki komunikatów
    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if (key == -1) {
        perror("[SPAWNER] Błąd ftok");
        exit(1);
    }
    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("[SPAWNER] Błąd msgget (czy kasjer wystartował?)");
        exit(1);
    }

    // Rejestracja spawnera
    struct msgbuf_spawnerReg reg;
    reg.mtype      = 10;
    reg.spawnerPid = getpid();
    if (msgsnd(msgid, &reg, sizeof(reg) - sizeof(long), 0) == -1) {
        perror("[SPAWNER] Błąd msgsnd (spawner->kasjer)");
        exit(1);
    }

    // Tworzenie procesów grup
    int nextGroupId = 1;
    while (1) {
        int groupSize = 1 + (rand() % 3); // Losowanie wielkości grupy (1-3)
        pid_t pid = fork();
        if (pid == 0) {
            group_main_process(nextGroupId, groupSize); // Proces grupy
        } else if (pid > 0) {
            printf("\nUtworzono grupę #%d (%d-os.), PID=%d\n",
                   nextGroupId, groupSize, pid);
            nextGroupId++;

            // Przerwa między kolejnymi grupami
            int pauseSec = 3 + (rand() % 3);
            sleep(pauseSec); // Przerwa 3-5 sekund
        } else {
            perror("[SPAWNER] Błąd fork()");
            break;
        }
    }

    return 0;
}
