CC=gcc -Wall -I include/ -DWIN32
LDFLAGS=-Xlinker --subsystem -Xlinker windows

default: all

all: term.exe

clean:
	rm *.o term.exe

term.exe: term.o vt100.o draw.o termplay.coff formats.o arch.o stream_in.o
	${CC} -o term.exe term.o vt100.o draw.o termplay.coff formats.o arch.o stream_in.o -lgdi32 -lcomctl32 -llibbz2

vt100.o: vt100.c vt100.h charsets.h
	${CC} -c vt100.c

draw.o: draw.c vt100.c vt100.h draw.h
	${CC} -c draw.c

termplay.coff: termplay.rc termplay.ico termplay.manifest
	windres termplay.rc termplay.coff

arch.o: arch.c arch.h
	${CC} -c arch.c

formats.o: formats.c arch.h
	${CC} -c formats.c

stream_in.o: stream_in.c stream_in.h
	${CC} -c stream_in.c

term.o: term.c vt100.h draw.h arch.h stream_in.h
	${CC} -c term.c
