# Makefile for ICT374 Assignment 2 - Part 2
# Builds:
#   - server (server.c + auth.c)
#   - client (client.c)

CC      = gcc
CFLAGS  = -Wall -Wextra -g
LDFLAGS = 

# Object files
SERVER_OBJS = server.o auth.o
CLIENT_OBJS = client.o

# Default target: build both server and client
all: server client

# Build server
server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o server $(SERVER_OBJS) $(LDFLAGS)

# Build client
client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o client $(CLIENT_OBJS) $(LDFLAGS)

# Object file rules

server.o: server.c auth.h protocol.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c protocol.h
	$(CC) $(CFLAGS) -c client.c

auth.o: auth.c auth.h
	$(CC) $(CFLAGS) -c auth.c

# Clean up build artifacts
clean:
	rm -f server client *.o

.PHONY: all clean
