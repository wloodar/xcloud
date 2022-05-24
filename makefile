
CC     := clang
SRC    := $(wildcard *.c)
CFLAGS := -Wall -Wextra

server:
	rm -rf build
	mkdir -p build
	$(CC) -o build/xcloud-server $(CFLAGS) $(filter-out client.c, $(SRC))

compile:
	rm -rf build
	mkdir -p build
	$(CC) -o build/xcloud $(CFLAGS) $(SRC)

run: compile
	./build/xcloud

run-server: server
	./build/xcloud-server
