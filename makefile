
CC = gcc
CFLAGS = -Wall -Iinclude      # -Iinclude tells gcc where to find .h files
LDFLAGS = -lncurses           # Link ncurses library
OBJ = main.o token.o command.o
TARGET = ./bin/shell

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)
 
# test

test: test.o token.o command.o
	$(CC) test.o token.o command.o -o test $(LDFLAGS)

test.o : tests/test.c include/token.h include/command.h
	$(CC) $(CFLAGS) -c tests/test.c -o test.o

main.o: main.c include/token.h include/command.h
	$(CC) $(CFLAGS) -c main.c

token.o: token.c include/token.h
	$(CC) $(CFLAGS) -c token.c

command.o: command.c include/command.h
	$(CC) $(CFLAGS) -c command.c

clean:
	rm -f $(OBJ) $(TARGET)
