bin_PROGRAMS = baudrate term rec
AM_CFLAGS=-Wall

baudrate_SOURCES = baudrate.c
baudrate_LDADD = utils.o

if WIN32
term_SOURCES = term.c draw.c timeline.c timeline.h draw.h vt100.c vt100.h charsets.h
term_LDADD = stream.o compress.o formats.o no_record.o utils.o -lcomdlg32 -lcomctl32 -lgdi32 termplay.coff @LIBBZ2@ @LIBZ@
term_LDFLAGS = -Xlinker --subsystem -Xlinker windows
rec_SOURCES = winrec.c
rec_LDADD = no_play.o formats.o utils.o stream.o compress.o termrec.coff @LIBBZ2@ @LIBZ@
else
term_SOURCES = dumbplay.c
term_LDADD = stream.o compress.o formats.o no_record.o
rec_SOURCES = ttyr.c pty.c
rec_LDADD = stream.o compress.o formats.o no_play.o
endif

term_RES = termplay.rc termplay.ico termplay.manifest open.bmp rewind.bmp pause.bmp play.bmp
rec_RES = termrec.rc termrec.ico
COMMON = stream.c stream.h compress.c formats.c formats.h no_play.c no_record.c utils.c utils.h synch.h winutils.c
EXTRA_DIST= ${COMMON} ${term_RES} ${rec_RES}

DISTCLEANFILES = _stdint.h
CLEANFILES = termplay.coff termrec.coff

termplay.coff: termplay.rc ${term_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
termrec.coff: termrec.rc ${rec_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termrec.rc "$$builddir/termrec.coff"
