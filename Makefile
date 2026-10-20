CC = gcc
CFLAGS = -Wall -Wextra

all: server

server: main.o
	$(CC) $(CFLAGS) -o $@ $^

main.o: main.c base.h
	$(CC) $(CFLAGS) -c $< -o $@

# For testing, compile main.c with main() renamed so it doesn't conflict with test's main()
main_test.o: main.c base.h
	$(CC) $(CFLAGS) -Dmain=app_main -c $< -o $@

test_new_client.o: test_new_client.c base.h
	$(CC) $(CFLAGS) -c $< -o $@

test_runner: main_test.o test_new_client.o
	$(CC) $(CFLAGS) $^ -Wl,--wrap=accept -Wl,--wrap=recv -Wl,--wrap=send -o $@

test: test_runner
	./test_runner

clean:
	rm -f *.o server test_runner

.PHONY: all test clean
