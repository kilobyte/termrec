bin_PROGRAMS = termrec termplay
AM_CFLAGS = -Wall -I${srcdir}/../libtty -I${srcdir}/../ugly

termrec_SOURCES = ttyr.c pty.c
termrec_LDADD = ../ugly/name_out.o -L../libtty -ltty @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@

termplay_SOURCES = dumbplay.c
termplay_LDADD = ../ugly/name_in.o -L../libtty -ltty @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
