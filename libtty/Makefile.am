lib_LTLIBRARIES = libtty.la
libtty_la_SOURCES = vt100.c charsets.h compress.c stream.h formats.c formats.h stream.c stream.h utils.c synch.h timeline.c timeline.h
libtty_la_LIBADD = ../sys/libcompat.la @LIBBZ2@ @LIBZ@
libtty_la_LDFLAGS = -no-undefined
AM_CFLAGS = -Wall

include_HEADERS = vt100.h
