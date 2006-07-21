bin_PROGRAMS = termrec proxyrec termplay
AM_CFLAGS=-Wall

if WIN32
termplay_SOURCES = win32/term.c win32/draw.c timeline.c timeline.h win32/draw.h vt100.c name_in.c
termplay_LDADD_ARCH = -lcomdlg32 -lcomctl32 -lgdi32 termplay.coff
termplay_LDFLAGS = -Xlinker --subsystem -Xlinker windows
termrec_SOURCES = win32/winrec.c
termrec_LDADD_ARCH = termrec.coff
proxyrec_LDADD_ARCH = proxyrec.coff
proxyrec_LDFLAGS_ARCH = -Xlinker --subsystem -Xlinker windows
else
termplay_SOURCES = dumbplay.c name_in.c
termrec_SOURCES = unix/ttyr.c unix/pty.c
endif
termplay_LDADD = stream.o compress.o formats.o name_out.o utils.o ${termplay_LDADD_ARCH} @LIBBZ2@ @LIBZ@
termrec_LDADD = stream.o compress.o formats.o no_play.o name_out.o utils.o ${termrec_LDADD_ARCH} @LIBBZ2@ @LIBZ@
proxyrec_SOURCES = proxy.c
proxyrec_LDADD = no_play.o formats.o utils.o stream.o compress.o ${proxyrec_LDADD_ARCH} name_out.o @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
proxyrec_LDFLAGS = ${proxyrec_LDFLAGS_ARCH} @PTHREAD_CFLAGS@

termplay_RES = win32/termplay.rc win32/termplay.ico win32/termplay.manifest win32/open.bmp win32/rewind.bmp win32/pause.bmp win32/play.bmp win32/wood1.bmp win32/selstart.bmp win32/selend.bmp win32/export.bmp
termrec_RES = win32/termrec.rc win32/termrec.ico
proxyrec_RES = win32/proxyrec.rc win32/proxyrec.ico
CHEADERS = stream.h formats.h utils.h synch.h name_out.h threads.h unix/threads.h win32/threads.h pty.h name.h vt100.h charsets.h
COMMON = stream.c unix/stream.c win32/stream.c compress.c formats.c no_play.c utils.c name_out.c
EXTRA_DIST_W32 = win32/winutils.c ${termplay_RES} ${termrec_RES} ${proxyrec_RES}
EXTRA_DIST= ${CHEADERS} ${COMMON} BUGS ${EXTRA_DIST_W32}

DISTCLEANFILES = _stdint.h
CLEANFILES = termplay.coff termrec.coff proxyrec.coff

termplay.coff: ${termplay_RES}
	builddir=`pwd` && cd $(srcdir)/win32 && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
termrec.coff: ${termrec_RES}
	builddir=`pwd` && cd $(srcdir)/win32 && \
	  $(WINDRES) termrec.rc "$$builddir/termrec.coff"
proxyrec.coff: ${proxyrec_RES}
	builddir=`pwd` && cd $(srcdir)/win32 && \
	  $(WINDRES) proxyrec.rc "$$builddir/proxyrec.coff"
