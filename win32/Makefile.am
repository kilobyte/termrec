bin_PROGRAMS = termrec termplay
AM_CFLAGS=-Wall -I${srcdir}/../libtty/ -I${srcdir}/../ugly/

termplay_SOURCES = term.c draw.c draw.h
termplay_LDFLAGS = -Xlinker --subsystem -Xlinker windows
termrec_SOURCES = winrec.c
termplay_LDADD = ../sys/libcompat.la ../ugly/name_out.o -L../libtty -ltty -lcomdlg32 -lcomctl32 -lgdi32 termplay.coff icons/termplay.coff @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
termrec_LDADD = ../sys/libcompat.la ../ugly/name_out.o -L../libtty -ltty icons/termrec.coff @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@

termplay_RES = termplay.rc termplay.manifest open.bmp rewind.bmp pause.bmp play.bmp wood1.bmp selstart.bmp selend.bmp export.bmp
EXTRA_DIST= ${termplay_RES}

CLEANFILES = termplay.coff

termplay.coff: ${termplay_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
