bin_PROGRAMS = termrec proxyrec termplay
AM_CFLAGS=-Wall

if WIN32
termplay_SOURCES = term.c draw.c timeline.c timeline.h draw.h vt100.c name_in.c
termplay_LDADD = stream.o compress.o formats.o no_record.o utils.o -lcomdlg32 -lcomctl32 -lgdi32 termplay.coff @LIBBZ2@ @LIBZ@
termplay_LDFLAGS = -Xlinker --subsystem -Xlinker windows
termrec_SOURCES = winrec.c
termrec_LDADD_ARCH = termrec.coff
proxyrec_LDADD_ARCH = proxyrec.coff
proxyrec_LDFLAGS_ARCH = -Xlinker --subsystem -Xlinker windows
else
termplay_SOURCES = dumbplay.c name_in.c
termplay_LDADD = stream.o compress.o formats.o no_record.o utils.o
termrec_SOURCES = ttyr.c pty.c
endif
termrec_LDADD = stream.o compress.o formats.o no_play.o name_out.o utils.o ${termrec_LDADD_ARCH} @LIBBZ2@ @LIBZ@
proxyrec_SOURCES = proxy.c
proxyrec_LDADD = no_play.o formats.o utils.o stream.o compress.o ${proxyrec_LDADD_ARCH} name_out.o @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
proxyrec_LDFLAGS = ${proxyrec_LDFLAGS_ARCH} @PTHREAD_CFLAGS@

termplay_RES = termplay.rc termplay.ico termplay.manifest open.bmp rewind.bmp pause.bmp play.bmp wood1.bmp
termrec_RES = termrec.rc termrec.ico
proxyrec_RES = proxyrec.rc proxyrec.ico
CHEADERS = stream.h formats.h utils.h synch.h name_out.h threads.h pty.h name_in.h vt100.h charsets.h
COMMON = stream.c compress.c formats.c no_play.c no_record.c utils.c winutils.c name_out.c
EXTRA_DIST= ${CHEADERS} ${COMMON} ${termplay_RES} ${termrec_RES} ${proxyrec_RES} BUGS

DISTCLEANFILES = _stdint.h
CLEANFILES = termplay.coff termrec.coff proxyrec.coff

termplay.coff: termplay.rc ${termplay_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
termrec.coff: termrec.rc ${termrec_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termrec.rc "$$builddir/termrec.coff"
proxyrec.coff: proxyrec.rc ${proxyrec_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) proxyrec.rc "$$builddir/proxyrec.coff"
