CC=gcc -Wall -I include/ -DWIN32

default: all

all: pr.exe term.exe

clean:
	rm *.o pr.exe term.exe

pr.exe: pr.o vt100.o trend.o rend.o readpng_cyg.o
	${CC} -o pr.exe pr.o vt100.o trend.o rend.o readpng_cyg.o -lgdi32 -lcomdlg32

term.exe: term.o vt100.o draw.o termplay.coff formats.o arch.o
	${CC} -o term.exe term.o vt100.o draw.o termplay.coff formats.o arch.o -lgdi32

pr.o: pr.c vt100.h trend.h rend.h
	${CC} -c pr.c

vt100.o: vt100.c vt100.h charsets.h
	${CC} -c vt100.c

trend.o: trend.c trend.h
	${CC} -c trend.c

rend.o: rend.c trend.h rend.h vt100.h
	${CC} -c rend.c

draw.o: draw.c vt100.c vt100.h draw.h
	${CC} -c draw.c

readpng_cyg.o: readpng_cyg.c readpng.h trend.h
	${CC} -c readpng_cyg.c

termplay.coff: termplay.rc termplay.ico
	windres termplay.rc termplay.coff

arch.o: arch.c arch.h
	${CC} -c arch.c

formats.o: formats.c arch.h
	${CC} -c formats.c
