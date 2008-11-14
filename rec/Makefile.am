bin_PROGRAMS = termrec
AM_CFLAGS = -Wall -I${srcdir}/../sys -I${srcdir}/../libtty -I${srcdir}/../common

termrec_SOURCES = termrec.c pty.c pty.h
termrec_LDADD = ../sys/libutils.la ../common/libcommon.la -L../libtty -ltty

if WIN32
termrec_LDADD += ../win32/icons/termrec.coff
termrec_LDFLAGS = -Xlinker --subsystem -Xlinker windows
endif
