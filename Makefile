CC = gcc
CFLAGS = -Wall -pedantic -std=c11 -D_POSIX_C_SOURCE=200809L -pthread

OBJS = cashier.o client.o firefighter.o ipc_helpers.o 

all: cashier client firefighter

cashier: cashier.o ipc_helpers.o 
	$(CC) $(CFLAGS) -o cashier cashier.o ipc_helpers.o

client: client.o ipc_helpers.o 
	$(CC) $(CFLAGS) -o client client.o ipc_helpers.o

firefighter: firefighter.o
	$(CC) $(CFLAGS) -o firefighter firefighter.o

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o cashier client firefighter
