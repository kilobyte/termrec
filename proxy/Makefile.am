bin_PROGRAMS = proxyrec
AM_CFLAGS = -Wall -I${srcdir}/../libtty -I${srcdir}/../ugly

proxyrec_SOURCES = proxy.c
proxyrec_LDADD = ../sys/asprintf.o ../sys/libcompat.a ../libtty/libtty.a ../ugly/name_out.o @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
proxyrec_LDFLAGS = @PTHREAD_CFLAGS@

if WIN32
proxyrec_LDADD += ../win32/proxyrec.coff
proxyrec_LDFLAGS += -Xlinker --subsystem -Xlinker windows
endif
