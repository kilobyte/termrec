AM_CFLAGS=-Wall

if WIN32
SUBDIRS = sys libtty tmp ugly win32 proxy
else
SUBDIRS = sys libtty tmp ugly unix proxy
endif

EXTRA_DIST= BUGS gettext.h
DISTCLEANFILES = _stdint.h
