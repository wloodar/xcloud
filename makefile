
CC     := clang
CFLAGS := -Wall -Wextra -fsanitize=address -Iinclude

server:
	rm -rf build
	mkdir -p build
	$(CC) -o build/xcloud-server $(CFLAGS) $(wildcard server/*.c) \
		$(wildcard common/*.c)

compile:
	rm -rf build
	mkdir -p build
	$(CC) -o build/xcloud $(CFLAGS) $(SRC)

run: compile
	./build/xcloud

run-server: server
	./build/xcloud-server

.PHONY : server
