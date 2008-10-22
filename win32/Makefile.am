bin_PROGRAMS = termrec termplay
AM_CFLAGS=-Wall -I${srcdir}/../libtty/ -I${srcdir}/../ugly/

termplay_SOURCES = term.c draw.c draw.h
termplay_LDFLAGS = -Xlinker --subsystem -Xlinker windows
termrec_SOURCES = winrec.c
termplay_LDADD = ../sys/asprintf.o ../sys/libcompat.a ../ugly/name_out.o ../ugly/name_in.o ../libtty/libtty.a -lcomdlg32 -lcomctl32 -lgdi32 termplay.coff icons/termplay.coff @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
termrec_LDADD = ../sys/asprintf.o ../sys/libcompat.a ../ugly/name_out.o ../libtty/libtty.a icons/termrec.coff @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@

termplay_RES = termplay.rc termplay.manifest open.bmp rewind.bmp pause.bmp play.bmp wood1.bmp selstart.bmp selend.bmp export.bmp
CHEADERS = stream.h formats.h utils.h synch.h name_out.h threads.h unix/threads.h threads.h pty.h name.h vt100.h charsets.h net.h net.h winutils.h
EXTRA_DIST= ${CHEADERS} ${termplay_RES}

CLEANFILES = termplay.coff

termplay.coff: ${termplay_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
