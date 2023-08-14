all:
	$(CC) -Wall -Wextra -fsanitize=address -o xcloud-server src/server.c
	$(CC) -Wall -Wextra -fsanitize=address -o xcloud src/client.c src/common.c src/gui.c
