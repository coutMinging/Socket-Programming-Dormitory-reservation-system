CC = gcc
CFLAGS = -Wall -g

SERVER_EXEC = servermain
CLIENT_EXEC = client
SERVER_SRC = servermain.c
CLIENT_SRC = client.c

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_EXEC) $(SERVER_SRC)

$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_EXEC) $(CLIENT_SRC)

clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)