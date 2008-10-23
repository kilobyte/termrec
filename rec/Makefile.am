bin_PROGRAMS = termrec
AM_CFLAGS = -Wall -I${srcdir}/../libtty -I${srcdir}/../ugly

termrec_SOURCES = termrec.c pty.c pty.h
termrec_LDADD = ../sys/libcompat.la ../ugly/name_out.o -L../libtty -ltty @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
termrec_LDFLAGS = @PTHREAD_CFLAGS@

if WIN32
termrec_LDADD += ../win32/icons/termrec.coff
termrec_LDFLAGS += -Xlinker --subsystem -Xlinker windows
endif
