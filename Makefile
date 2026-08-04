CC = clang
CFLAGS = -Wall -Wextra -std=c99

all: main

main: main.o rbtree.o
	$(CC) $(CFLAGS) -o main main.o rbtree.o

main.o: main.c rbtree.h
	$(CC) $(CFLAGS) -c main.c

rbtree.o: rbtree.c rbtree.h
	$(CC) $(CFLAGS) -c rbtree.c

# ── Test target ──────────────────────────────────────────────────
test_rbtree: test_rbtree.o rbtree.o
	$(CC) $(CFLAGS) -o test_rbtree test_rbtree.o rbtree.o

test_rbtree.o: test_rbtree.c rbtree.h
	$(CC) $(CFLAGS) -c test_rbtree.c

test: test_rbtree
	./test_rbtree

clean:
	rm -f main main.o rbtree.o test_rbtree test_rbtree.o