bin_PROGRAMS = termrec termplay
AM_CFLAGS = -Wall -I${srcdir}/../libtty -I${srcdir}/../ugly

termrec_SOURCES = ttyr.c pty.c
termrec_LDADD = ../libtty/libtty.a ../ugly/name_out.o @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@

termplay_SOURCES = dumbplay.c
termplay_LDADD = ../libtty/libtty.a ../ugly/name_in.o @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
