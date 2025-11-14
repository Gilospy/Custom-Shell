CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = myshell
OBJS = main.o parser.o execute.o builtins.o history.o signals.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c shell.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c shell.h
	$(CC) $(CFLAGS) -c parser.c

execute.o: execute.c shell.h
	$(CC) $(CFLAGS) -c execute.c

builtins.o: builtins.c shell.h
	$(CC) $(CFLAGS) -c builtins.c

history.o: history.c shell.h
	$(CC) $(CFLAGS) -c history.c

signals.o: signals.c shell.h
	$(CC) $(CFLAGS) -c signals.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
