CC = gcc
CFLAGS = -pthread

all: server

server: main.o grading_queue.o compiler.o network.o
	$(CC) $(CFLAGS) -o server main.o grading_queue.o compiler.o network.o

main.o: main.c grading_queue.h compiler.h network.h
	$(CC) $(CFLAGS) -c main.c

grading_queue.o: grading_queue.c grading_queue.h
	$(CC) $(CFLAGS) -c grading_queue.c

compiler.o: compiler.c compiler.h
	$(CC) $(CFLAGS) -c compiler.c

network.o: network.c network.h grading_queue.h compiler.h
	$(CC) $(CFLAGS) -c network.c

clean:
	rm -f server main.o grading_queue.o compiler.o network.o
