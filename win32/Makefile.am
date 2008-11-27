bin_PROGRAMS += termrec termplay

termplay_SOURCES = win32/term.c win32/draw.c win32/draw.h
termplay_LDFLAGS = -Xlinker --subsystem -Xlinker windows
termrec_SOURCES = win32/winrec.c
termplay_LDADD = libutils.la libtty.la -lcomdlg32 -lcomctl32 -lgdi32 win32/icons/termplay.coff @LIBBZ2@ @LIBZ@ @PTHREAD_LIBS@
termrec_LDADD = libcommon.la libutils.la libtty.la win32/icons/termrec.coff
