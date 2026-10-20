CC = gcc
CFLAGS = -Wall -Wextra

all: main

main: main.c
	$(CC) $(CFLAGS) -o main main.c

test: test_main.c
	$(CC) $(CFLAGS) -o test_main test_main.c
	./test_main

clean:
	rm -f main test_main *.o
