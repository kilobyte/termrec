bin_PROGRAMS += termrec

termrec_SOURCES = rec/termrec.c rec/pty.c rec/pty.h
termrec_LDADD = libutils.la libcommon.la libtty.la

if WIN32
termrec_LDADD += win32/icons/termrec.coff
termrec_LDFLAGS = -Xlinker --subsystem -Xlinker windows
endif
