
CC     := clang
SRC    := $(wildcard *.c)
CFLAGS := -Wall -Wextra


compile:
	rm -rf build
	mkdir -p build
	$(CC) -o build/xcloud $(CFLAGS) $(SRC)

run: compile
	./build/xcloud
