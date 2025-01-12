#include "common.h"


int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr, "Wprowadź dane w formacie: %s <PID_kasjera>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t cashierPid = (pid_t)atoi(argv[1]);
    if (cashierPid <= 0) {
        fprintf(stderr, "Niepoprawny PID kasjera.\n");
        exit(EXIT_FAILURE);
    }

    printf("[STRAŻAK] Wysyłam sygnał pożaru do kasjera (PID=%d)\n", cashierPid);
    if(kill(cashierPid, FIRE_SIGNAL) == -1){
        perror("Błąd: Nie udało się wysłać sygnału pożaru do kasjera. Upewnij się, że podany PID jest poprawny i że proces ma odpowiednie uprawnienia.");
        exit(EXIT_FAILURE);
    }

}