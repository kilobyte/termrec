AM_CFLAGS=-Wall

SUBDIRS = sys libstream libtty

if WIN32
SUBDIRS += win32/icons
endif

SUBDIRS += tmp ugly proxy

if WIN32
SUBDIRS += win32
else
SUBDIRS += play rec
endif

EXTRA_DIST= BUGS gettext.h
DISTCLEANFILES = _stdint.h
