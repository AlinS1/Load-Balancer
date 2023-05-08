CC=gcc
CFLAGS=-g -std=c99 -Wall -Wextra
LOAD=load_balancer
SERVER=server
LIST_HT=list_ht

.PHONY: build clean

build: tema2

tema2: main.o $(LOAD).o $(SERVER).o $(LIST_HT).o
	$(CC) $^ -o $@

main.o: main.c
	$(CC) $(CFLAGS) $^ -c

$(SERVER).o: $(SERVER).c $(SERVER).h
	$(CC) $(CFLAGS) $^ -c

$(LOAD).o: $(LOAD).c $(LOAD).h
	$(CC) $(CFLAGS) $^ -c

$(LIST_HT).o: $(LIST_HT).c $(LIST_HT).h
	$(CC) $(CFLAGS) $^ -c

clean:
	rm -f *.o tema2 *.h.gch
