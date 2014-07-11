lib_LTLIBRARIES = libtty.la

libtty_la_SOURCES = $(LIBSTREAM) \
	libtty/tty.c \
	libtty/charsets.c \
	libtty/charsets.h \
	libtty/formats.c \
	libtty/formats.h \
	libtty/timeline.c \
	libtty/files.c \
	libtty/vtredir.c \
	libtty/dosrecorder.c
libtty_la_LIBADD = libcompat.la libttyutils.la @LIBBZ2@ @LIBZ@ @LIBLZMA@
libtty_la_LDFLAGS = -no-undefined

#disabled: if !HAVE_FORK
libtty_la_LIBADD += @PTHREAD_LIBS@
libtty_la_LDFLAGS += @PTHREAD_CFLAGS@
#endif

include_HEADERS = libtty/tty.h libtty/ttyrec.h
