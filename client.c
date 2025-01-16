#include "common.h"
#include <pthread.h>
#include <time.h>


// Obsługa sygnału pożaru
static void client_fire_handler(int signo) {
    if (signo == FIRE_SIGNAL) {
        printf("[KLIENT %d] Klienci opuszczają pizzerie\n", getpid());
        exit(0);
    }
}

// Struktura opisująca jedną grupę (dzielona przez wątki w grupie)
typedef struct {
    int groupId;      
    int groupSize;    
    int tableSize;    
    bool canSit;      
    bool responseReady;
    int eatTime;      
    pthread_mutex_t lock;
    pthread_cond_t  cond;
} GroupInfo;

//Wątek zwykły (członek grupy)
void* client_thread_func(void* arg) {
    GroupInfo* g = (GroupInfo*)arg;

    // Czekamy, aż lider ustali, czy mamy stolik, i ustawi responseReady
    pthread_mutex_lock(&g->lock);
    while (!g->responseReady) {
        pthread_cond_wait(&g->cond, &g->lock);
    }

    bool localCanSit = g->canSit;
    int  localTable  = g->tableSize;
    int  localGId    = g->groupId;
    //int  localSize   = g->groupSize;
    int  localEat    = g->eatTime;    
    pthread_mutex_unlock(&g->lock);

    // Jeśli brak stolika -> kończymy się
    if (!localCanSit) {
        pthread_exit(NULL);
    } else {
        printf("[CZLONEK GRUPY %d] Jem przy stoliku %d-os. %d s\n",
               localGId,localTable, localEat);
        sleep(localEat);

        // Wątek kończy się, lider zwolni stolik
        pthread_exit(NULL);
    }
}

//Wątek lidera
void* leader_thread_func(void* arg) {
    GroupInfo* g = (GroupInfo*)arg;

    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if (key == -1) {
        perror("[LEADER] Błąd: Nie udało się wygenerować klucza IPC za pomocą ftok");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->tableSize = 0;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    int msgid = msgget(key, 0666);
    if (msgid == -1) {
        perror("[LEADER] Błąd: Nie udało się połączyć z kolejką komunikatów za pomocą msgget");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->tableSize = 0;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    // Wysyłamy zapytanie (mtype=1)
    struct msgbuf_request req;
    req.mtype     = 1;
    req.groupId   = g->groupId;   // ID grupy
    req.groupSize = g->groupSize; // 1..3
    req.pidClient = getpid();     // PID procesu klienta

    if (msgsnd(msgid, &req, sizeof(req) - sizeof(long), 0) == -1) {
        perror("[LEADER] Błąd: Nie udało się wysłać zapytania za pomocą msgsnd");
        if((errno == EAGAIN)){
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->tableSize = 0;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    // Odbieramy odpowiedź (mtype=2)
    struct msgbuf_response resp;
    if (msgrcv(msgid, &resp, sizeof(resp) - sizeof(long), 2, 0) == -1) { 
        perror("[LEADER] Błąd: Nie udało się odebrać odpowiedzi za pomocą msgrcv");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->tableSize = 0;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    // Aktualizujemy w strukturze info o przydziale
    pthread_mutex_lock(&g->lock);
    g->canSit       = resp.canSit;
    g->tableSize    = resp.tableSize;  // 1..4
    g->responseReady = true;

    // Jeśli kasjer przydzielił stolik, losujemy czas jedzenia (np. 2..5 s)
    if (resp.canSit) {
        g->eatTime = 7 + (rand() % 6);
    } else {
        g->eatTime = 0;
    }

    // Pobudka innych wątków grupy
    pthread_cond_broadcast(&g->cond);
    pthread_mutex_unlock(&g->lock);

    if (!resp.canSit) {
        // brak stolika
        pthread_exit(NULL);
    } else {
        // Mamy stolik
        int localEat;
        pthread_mutex_lock(&g->lock);
        localEat = g->eatTime;
        pthread_mutex_unlock(&g->lock);

        printf("\n\n[LIDER GRUPY %d] (gr %d-os.) Otrzymała stolik %d-os. Jem %d s.\n",
               g->groupId, g->groupSize, resp.tableSize, localEat);
        sleep(localEat);

        // Po jedzeniu lider zwalnia stolik
        struct msgbuf_release rel;
        rel.mtype     = 3;
        rel.groupId   = g->groupId;
        rel.tableSize = resp.tableSize;
        rel.tableIndex= resp.tableIndex; 
        rel.groupSize = g->groupSize;
        rel.pidClient = getpid();

        if (msgsnd(msgid, &rel, sizeof(rel) - sizeof(long), 0) == -1) {
            perror("[LEADER] zwalnianie stolika");
        }
        pthread_exit(NULL);
    }
}

// Zmienna globalna do nadawania ID grup
static int nextGroupId = 1;

// Generowanie jednej grupy
void generate_one_group() {
    // Losujemy rozmiar grupy: 1..3
    int groupSize = 1 + (rand() % 3);

    // Alokujemy strukturę grupy
    GroupInfo* g = malloc(sizeof(GroupInfo));
    g->groupId   = nextGroupId++;
    g->groupSize = groupSize;
    g->tableSize = 0;
    g->canSit    = false;
    g->responseReady = false;
    g->eatTime   = 0;  

    pthread_mutex_init(&g->lock, NULL);
    pthread_cond_init(&g->cond, NULL);

    // Tworzymy wątki
    pthread_t* tids = calloc(groupSize, sizeof(pthread_t));

    // Pierwszy = lider
    pthread_create(&tids[0], NULL, leader_thread_func, g);

    // Reszta
    for (int i = 1; i < groupSize; i++) {
        pthread_create(&tids[i], NULL, client_thread_func, g);
    }

    // Odłączamy wątki, by nie blokować joinowania
    for (int i = 0; i < groupSize; i++) {
        pthread_detach(tids[i]);
    }
    free(tids);
}

int main(int argc, char* argv[]) {
    // Obsługa sygnału (pożar)
    struct sigaction sa;
    sa.sa_handler = client_fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);

    srand(time(NULL));

    printf("[KLIENT %d] Grupy klientów zaczynają przychodzić...\n\n", getpid());

    // Nieskończona pętla
    while (1) {
        generate_one_group();
        
        // Przerwa między przyjściami grup
        int pauseSec = 3 + (rand() % 3);
        sleep(pauseSec);
    }
    return 0;
}
