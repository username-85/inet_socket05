CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -g -O0

.PHONY: default all clean

SERVER_TARGET = server
SERVER_SRC = server.c 
SERVER_OBJ = $(SERVER_SRC:.c=.o)

CLIENT_TARGET = client
CLIENT_SRC = client.c
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

COMMON_SRC = util.c
COMMON_HEADERS = $(COMMON_SRC:.c=.h) common.h
COMMON_OBJ = $(COMMON_SRC:.c=.o)

default: all
all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(COMMON_OBJ): $(COMMON_SRC) $(COMMON_HEADERS) 
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_OBJ): $(SERVER_SRC) $(SERVER_HEADERS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_OBJ): $(CLIENT_SRC) $(CLIENT_HEADERS) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_TARGET): $(SERVER_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJ) $(COMMON_OBJ)

$(CLIENT_TARGET): $(CLIENT_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJ) $(COMMON_OBJ)

clean:
	-rm -f *.o
	-rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
