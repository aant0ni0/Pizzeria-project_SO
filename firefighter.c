#include "common.h"

int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr, "Wprowadź dane w formacie: %s <PID_kasjera>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

}