
CC     := clang
CFLAGS := -Wall -Wextra -fsanitize=address -Iinclude

server:
	rm -rf build
	mkdir -p build
	$(CC) -o build/xcloud-server $(CFLAGS) $(wildcard server/*.c) \
		$(wildcard common/*.c)

daemon:
	rm -rf build
	mkdir -p build
	$(CC) -o build/daemon $(CFLAGS) $(wildcard daemon/*.c) $(wildcard common/*.c)

client:
	rm -rf build
	mkdir -p build
	$(CC) -o build/xcloud $(CFLAGS) $(wildcard client/*.c) $(wildcard common/*.c)

run-server: server
	./build/xcloud-server

run-daemon: daemon
	./build/daemon

run-client: client
	./build/xcloud

.PHONY : server client daemon
