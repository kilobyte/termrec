lib_LTLIBRARIES = libtty.la
AM_CFLAGS = -Wall -I${srcdir}/../sys

libtty_la_SOURCES = vt100.c charsets.h formats.c formats.h synch.h timeline.c timeline.h files.c files.h
libtty_la_LIBADD = ../sys/libcompat.la ../libstream/libstream.la @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
libtty_la_LDFLAGS = -no-undefined @PTHREAD_CFLAGS@

include_HEADERS = vt100.h ttyrec.h
