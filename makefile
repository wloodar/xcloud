all:
	$(CC) -o xcloud-server src/server.c
	$(CC) -o xcloud src/client.c
