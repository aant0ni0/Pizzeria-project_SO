#include "common.h"
#include "ipc_helpers.h"
#include "sharedmem_helpers.h"

int main(int argc, char* argv[]){
    if(argc < 3){
        fprintf(stderr, "Użycie: %s <rozmiar_grupy> <czas_jedzenia_s>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int groupSize = atoi(argv[1]);
    int eatTime = atoi(argv[2]);

    if (groupSize < 1 || groupSize > MAX_GROUP_SIZE) {
        fprintf(stderr, "[KLIENT] Niepoprawny rozmiar grupy\n");
        exit(EXIT_FAILURE);
    }   
}