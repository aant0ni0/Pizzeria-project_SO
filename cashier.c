#include "common.h"
#include <ctype.h>

static volatile sig_atomic_t fireHappened 0;

void fire_handler(int signo){
    if(signo == FIRE_SINGAL){
        fireHappened = 1;
    }
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


}