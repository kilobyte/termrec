lib_LTLIBRARIES = libtty.la
libtty_la_SOURCES = vt100.c charsets.h compress.c stream.h formats.c formats.h stream.c stream.h utils.c synch.h timeline.c timeline.h
libtty_la_LIBADD = @LIBBZ2@ @LIBZ@
AM_CFLAGS = -Wall -fvisibility=hidden

include_HEADERS = vt100.h
