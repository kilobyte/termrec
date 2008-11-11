bin_PROGRAMS = termrec
AM_CFLAGS = -Wall -I${srcdir}/../sys -I${srcdir}/../libtty -I${srcdir}/../common

termrec_SOURCES = termrec.c pty.c pty.h
termrec_LDADD = ../sys/libcompat.la ../common/libcommon.la -L../libtty -ltty @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
termrec_LDFLAGS = @PTHREAD_CFLAGS@

if WIN32
termrec_LDADD += ../win32/icons/termrec.coff
termrec_LDFLAGS += -Xlinker --subsystem -Xlinker windows
endif
