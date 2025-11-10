
CC = gcc
CFLAGS = -Wall -Iinclude     # -Iinclude tells gcc where to find .h files
OBJ = main.o token.o command.o
TARGET = lab07_ex3

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

main.o: main.c include/token.h include/command.h
	$(CC) $(CFLAGS) -c main.c

token.o: token.c include/token.h
	$(CC) $(CFLAGS) -c token.c

command.o: command.c include/command.h
	$(CC) $(CFLAGS) -c command.c

clean:
	rm -f $(OBJ) $(TARGET)
