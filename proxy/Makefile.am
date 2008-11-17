bin_PROGRAMS = proxyrec
AM_CFLAGS = -Wall -I${srcdir}/../libtty -I${srcdir}/../sys

proxyrec_SOURCES = proxy.c
proxyrec_LDADD = ../common/libcommon.la ../sys/libutils.la -L../libtty -ltty @PTHREAD_LIBS@
proxyrec_LDFLAGS = @PTHREAD_CFLAGS@

if WIN32
proxyrec_LDADD += ../win32/icons/proxyrec.coff
proxyrec_LDFLAGS += -Xlinker --subsystem -Xlinker windows
endif
