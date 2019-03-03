.PHONY: all

all: proj1

proj1: main.o
	gcc main.o -lelf -o proj1

main.o: main.c
	gcc main.c -c -o main.o

clean:
	rm -rf proj1
	rm -rf main.o
