lib_LTLIBRARIES = libtty.la
AM_CFLAGS = -Wall -I${srcdir}/../sys

libtty_la_SOURCES = vt100.c charsets.h formats.c formats.h timeline.c files.c
libtty_la_LIBADD = ../sys/libcompat.la ../libstream/libstream.la @LIBBZ2@ @LIBZ@
libtty_la_LDFLAGS = -no-undefined

if !HAVE_FORK
libtty_la_LIBADD += @PTHREAD_LIBS@
libtty_la_LDFLAGS += @PTHREAD_CFLAGS@
endif

include_HEADERS = vt100.h ttyrec.h
