bin_PROGRAMS = termrec termplay

termplay_SOURCES = win32/term.c win32/draw.c win32/draw.h
termplay_LDFLAGS = -Xlinker --subsystem -Xlinker windows
termrec_SOURCES = win32/winrec.c
termplay_LDADD = libutils.la libtty.la -lcomdlg32 -lcomctl32 -lgdi32 termplay.coff icons/termplay.coff @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
termrec_LDADD = libcommon.la libutils.la libtty.la icons/termrec.coff

WIN32RES = termplay.rc termplay.manifest open.bmp rewind.bmp pause.bmp play.bmp wood1.bmp selstart.bmp selend.bmp export.bmp
termplay_RES = $(WIN32RES:%=win32/%)
EXTRA_DIST= ${termplay_RES} win32/lib/README

CLEANFILES = termplay.coff

termplay.coff: ${termplay_RES}
	builddir=`pwd` && cd $(srcdir/win32) && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
