all:
	$(CC) -o xcloud-server src/server.c
	$(CC) -fsanitize=address -o xcloud src/client.c
