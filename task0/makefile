all: client server

client: line_parser.o common.o client.o
	gcc -g -Wall -o client line_parser.o common.o client.o

server: line_parser.o common.o server.o
	gcc -g -Wall -o server line_parser.o common.o server.o

client.o: client.c
	gcc -g -Wall -c -o client.o client.c

server.o: server.c
	gcc -g -Wall -c -o server.o server.c

line_parser.o: line_parser.c line_parser.h
	gcc -g -Wall -c -o line_parser.o line_parser.c

common.o: common.c common.h
	gcc -g -Wall -c -o common.o common.c

.PHONY: clean

clean: 
	rm -f *.o client server
