bin_PROGRAMS = term rec proxy
AM_CFLAGS=-Wall

if WIN32
term_SOURCES = term.c draw.c timeline.c timeline.h draw.h vt100.c vt100.h charsets.h
term_LDADD = stream.o compress.o formats.o no_record.o utils.o -lcomdlg32 -lcomctl32 -lgdi32 termplay.coff @LIBBZ2@ @LIBZ@
term_LDFLAGS = -Xlinker --subsystem -Xlinker windows
rec_SOURCES = winrec.c name_out.c
rec_LDADD = no_play.o formats.o utils.o stream.o compress.o termrec.coff @LIBBZ2@ @LIBZ@
#proxy_LDFLAGS = -Xlinker --subsystem -Xlinker windows
else
term_SOURCES = dumbplay.c
term_LDADD = stream.o compress.o formats.o no_record.o
rec_SOURCES = ttyr.c pty.c name_out.c
rec_LDADD = stream.o compress.o formats.o no_play.o
endif
proxy_SOURCES = logproxy.c
proxy_LDADD = no_play.o formats.o utils.o stream.o compress.o logproxy.coff name_out.o @LIBBZ2@ @LIBZ@

term_RES = termplay.rc termplay.ico termplay.manifest open.bmp rewind.bmp pause.bmp play.bmp wood1.bmp
rec_RES = termrec.rc termrec.ico
proxy_RES = logproxy.rc logproxy.ico
COMMON = stream.c stream.h compress.c formats.c formats.h no_play.c no_record.c utils.c utils.h synch.h winutils.c name_out.h threads.h
EXTRA_DIST= ${COMMON} ${term_RES} ${rec_RES} ${proxy_RES}

DISTCLEANFILES = _stdint.h
CLEANFILES = termplay.coff termrec.coff logproxy.coff

termplay.coff: termplay.rc ${term_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
termrec.coff: termrec.rc ${rec_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termrec.rc "$$builddir/termrec.coff"
logproxy.coff: logproxy.rc ${logproxy_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) logproxy.rc "$$builddir/logproxy.coff"
