CC=i586-mingw32msvc-gcc -Wall -I include/

default: all

all: pr.exe

clean:
	rm *.o pr.exe

pr.exe: pr.o vt100.o trend.o rend.o readpng.o
	${CC} -o pr.exe pr.o vt100.o trend.o rend.o readpng.o libpng.lib -lgdi32 -lcomdlg32

pr.o: pr.c vt100.h trend.h rend.h
	${CC} -c pr.c

vt100.o: vt100.c vt100.h
	${CC} -c vt100.c

trend.o: trend.c trend.h
	${CC} -c trend.c

rend.o: rend.c trend.h rend.h vt100.h
	${CC} -c rend.c

readpngg.o: readpng.c readpng.h trend.h
	${CC} -c readpng.c

readpng_cyg.o: readpng_cyg.c readpng.h trend.h
	${CC} -c readpng_cyg.c
