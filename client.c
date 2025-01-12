#include "common.h"
#include <pthread.h>
#include <time.h>

// Obsługa sygnału pożaru
static void client_fire_handler(int signo) {
    if (signo == FIRE_SIGNAL) {
        printf("[KLIENT %d] Otrzymałem sygnał pożaru -> kończę się!\n", getpid());
        exit(0);
    }
}

// Struktura opisująca grupę (używana przez wątki w obrębie danej grupy)
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

// Wątek zwykły (członek grupy)
void* client_thread_func(void* arg) {
    GroupInfo* g = (GroupInfo*)arg;

    pthread_mutex_lock(&g->lock);
    while (!g->responseReady) {
        pthread_cond_wait(&g->cond, &g->lock); //odblokowanie mutexu i czekanie na sygnal lidera
    }

    bool localCanSit = g->canSit;
    int  localTable  = g->tableSize;
    int  localGId    = g->groupId;
    int  localSize   = g->groupSize;
    int  localEat    = g->eatTime;    
    pthread_mutex_unlock(&g->lock);

    if (!localCanSit) {
        // Brak stolika
        pthread_exit(NULL);
    } else {
        printf("[CZLONEK GRUPY %ld] (gr %d-os.) Jem przy stoliku %d-os. %d s\n",
               localGId,localSize, localTable, localEat);
        sleep(localEat);

        pthread_exit(NULL);
    }
}

// Wątek lider (pierwsza osoba w grupie)
void* leader_thread_func(void* arg) {
    GroupInfo* g = (GroupInfo*)arg;

    // Łączymy się z kolejką
    key_t key = ftok(PROJECT_PATH, PROJECT_ID);
    if (key == -1) {
        perror("[LEADER] ftok");
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
        perror("[LEADER] msgget");
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
    req.groupId   = g->groupId;
    req.groupSize = g->groupSize;
    req.pidClient = getpid();

    if (msgsnd(msgid, &req, sizeof(req) - sizeof(long), 0) == -1) {
        perror("[LEADER] msgsnd request");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->tableSize = 0;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    // czekamy na odpowiedź kasjera (mtype=2)
    struct msgbuf_response resp;
    if (msgrcv(msgid, &resp, sizeof(resp) - sizeof(long), 2, 0) == -1) { 
        perror("[LEADER] msgrcv response");
        pthread_mutex_lock(&g->lock);
        g->canSit = false;
        g->tableSize = 0;
        g->responseReady = true;
        pthread_cond_broadcast(&g->cond);
        pthread_mutex_unlock(&g->lock);
        pthread_exit(NULL);
    }

    // Ustawiamy dane w strukturze
    pthread_mutex_lock(&g->lock);
    g->canSit       = resp.canSit;
    g->tableSize    = resp.tableSize;
    g->responseReady = true;

    if (resp.canSit) {
        g->eatTime = 2 + (rand() % 20);  // np. 2..5 sekund
    } else {
        g->eatTime = 0;
    }

    // Pobudka pozostałych wątków
    pthread_cond_broadcast(&g->cond);
    pthread_mutex_unlock(&g->lock);

    // Jeśli brak stolika -> wyjście
    if (!resp.canSit) {
        printf("[LEADER %ld] Grupa #%d (%d-os.) NIE otrzymała stolika.\n",
               pthread_self(), g->groupId, g->groupSize);
        pthread_exit(NULL);
    } else {
        int eatTime;
        pthread_mutex_lock(&g->lock);
        eatTime = g->eatTime;
        pthread_mutex_unlock(&g->lock);

        printf("[LIDER GRUPY %d] (gr %d-os.) Grupa otrzymała stolik %d-os. Jem %d s.\n",
            g->groupId, g->groupSize, resp.tableSize, eatTime);
        sleep(eatTime);

        // Po jedzeniu zwalniamy stolik
        struct msgbuf_release rel;
        rel.mtype     = 3;
        rel.groupId   = g->groupId;
        rel.tableSize = resp.tableSize;
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

// Funkcja generująca jedną grupę
void generate_one_group() {
    int groupSize = 1 + (rand() % 3);

    GroupInfo* g = malloc(sizeof(GroupInfo));
    g->groupId   = nextGroupId++;
    g->groupSize = groupSize;
    g->tableSize = 0;
    g->canSit    = false;
    g->responseReady = false;
    g->eatTime   = 0;  // domyślnie

    pthread_mutex_init(&g->lock, NULL);
    pthread_cond_init(&g->cond, NULL);

    // Tablica wątków
    pthread_t* tids = calloc(groupSize, sizeof(pthread_t));

    // Lider
    pthread_create(&tids[0], NULL, leader_thread_func, g);

    // Reszta
    for (int i = 1; i < groupSize; i++) {
        pthread_create(&tids[i], NULL, client_thread_func, g);
    }

    // Detach
    for (int i = 0; i < groupSize; i++) {
        pthread_detach(tids[i]);
    }
    free(tids);
}

int main(int argc, char* argv[]) {
    // Sygnał pożaru
    struct sigaction sa;
    sa.sa_handler = client_fire_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(FIRE_SIGNAL, &sa, NULL);

    srand(time(NULL));

    printf("[KLIENT %d] Start. Generuję grupy w pętli...\n", getpid());

    // Nieskończona pętla: co pewien czas tworzona jest nowa grupa
    while (1) {
        generate_one_group();

        printf("\n\n");
        
        // przerwa
        int pauseSec = 2 + (rand() % 4);
        sleep(pauseSec);
    }
    return 0;
}
