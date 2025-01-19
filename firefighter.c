#include "common.h"

int main(int argc, char* argv[]) {
    // Sprawdzenie, czy podano odpowiednią liczbę argumentów
    if (argc < 2) {
        fprintf(stderr, "Użycie: %s <PID_kasjera>\n", argv[0]); // Wyświetlenie poprawnego sposobu użycia
        exit(EXIT_FAILURE); // Zakończenie programu w przypadku braku argumentu
    }

    // Konwersja argumentu na PID kasjera
    pid_t cashierPid = (pid_t)atoi(argv[1]); 
    if (cashierPid <= 0) { // Sprawdzenie poprawności PID
        fprintf(stderr, "Niepoprawny PID kasjera.\n"); // Komunikat o błędzie dla niepoprawnego PID
        exit(EXIT_FAILURE); // Zakończenie programu w przypadku błędnego PID
    }

    // Wysyłanie sygnału pożaru do procesu kasjera
    printf("[STRAŻAK] Wysyłam sygnał pożaru do kasjera (PID=%d)\n", cashierPid);
    if (kill(cashierPid, FIRE_SIGNAL) == -1) { // Próba wysłania sygnału
        perror("[STRAŻAK] Błąd kill"); // Komunikat o błędzie w przypadku niepowodzenia
        exit(EXIT_FAILURE); // Zakończenie programu w przypadku błędu
    }

    return 0; // Sukces - program zakończył się poprawnie
}
