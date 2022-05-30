
CC     := clang
CFLAGS := -Wall -Wextra -Iinclude -fsanitize=thread

all: server daemon client

server:
	mkdir -p build
	$(CC) -o build/xcloud-server $(CFLAGS) $(wildcard server/*.c) \
		$(wildcard common/*.c)

daemon:
	mkdir -p build
	$(CC) -o build/xcloud-daemon $(CFLAGS) $(wildcard daemon/*.c) $(wildcard common/*.c)

client:
	mkdir -p build
	$(CC) -o build/xcloud $(CFLAGS) $(wildcard client/*.c) $(wildcard common/*.c)

run-server: server
	./build/xcloud-server

run-daemon: daemon
	./build/xcloud-daemon

run-client: client
	./build/xcloud

clean:
	rm -rf build

.PHONY : server client daemon
