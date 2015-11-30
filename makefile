main: main.o config.o stats.o dnsserver.o lists.o
	gcc -o dnsserver -pthread -D_REENTRANT main.o config.o stats.o dnsserver.o lists.o -Wall
main.o: main.c global.h
	gcc -c main.c -Wall
config.o: config.c global.h
	gcc -c config.c -Wall
stats.o: stats.c global.h
	gcc -c stats.c -Wall
dnsserver.o: dnsserver.c global.h
	gcc -c dnsserver.c -Wall
lists.o: lists.c global.h
	gcc -c lists.c -Wall