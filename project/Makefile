cc = gcc
ccflags = -g -I. -std=gnu99 -Wall -pthread

all: server client

server: server.o comm.o db.o 
	$(cc) ${ccflags} $^ -o $@

server.o: server.c comm.h db.h
	$(cc) $< -c ${ccflags} -o $@

comm.o: comm.c comm.h
	$(cc) $< -c ${ccflags} -o $@

db.o: db.c db.h
	$(cc) $< -c ${ccflags} -o $@

client: client.c
	$(cc) -o $@ $< ${ccflags}

clean:
	/bin/rm -f *.o server client
