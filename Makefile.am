AM_CFLAGS=-Wall

SUBDIRS = sys libstream libtty

if WIN32
SUBDIRS += win32/icons
endif

SUBDIRS += common tmp proxy

if WIN32
SUBDIRS += win32
else
SUBDIRS += play rec time
endif

SUBDIRS += tests

EXTRA_DIST= BUGS autogen gettext.h export.h
DISTCLEANFILES = _stdint.h
