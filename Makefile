CC = clang
CFLAGS = -Wall -Wextra -std=c99

all: main

main: main.o rbtree.o
	$(CC) $(CFLAGS) -o main main.o rbtree.o

main.o: main.c rbtree.h
	$(CC) $(CFLAGS) -c main.c

rbtree.o: rbtree.c rbtree.h
	$(CC) $(CFLAGS) -c rbtree.c

clean:
	rm -f main main.o rbtree.o