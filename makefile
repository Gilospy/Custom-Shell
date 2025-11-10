# Simple Makefile for lab07 - command separation

lab07_ex3: main.o token.o command.o
	gcc main.o token.o command.o -o lab07_ex3

main.o: main.c token.h command.h
	gcc -c main.c

token.o: token.c token.h
	gcc -c token.c

command.o: command.c command.h
	gcc -c command.c

clean:
	rm *.o
