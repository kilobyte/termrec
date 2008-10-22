bin_PROGRAMS = proxyrec
AM_CFLAGS = -Wall -I${srcdir}/../libtty -I${srcdir}/../ugly

proxyrec_SOURCES = proxy.c
proxyrec_LDADD = ../sys/asprintf.o ../sys/libcompat.a ../ugly/name_out.o -L../libtty -ltty @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
proxyrec_LDFLAGS = @PTHREAD_CFLAGS@

if WIN32
proxyrec_LDADD += ../win32/icons/proxyrec.coff
proxyrec_LDFLAGS += -Xlinker --subsystem -Xlinker windows
endif
