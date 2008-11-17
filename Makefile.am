SUBDIRS = tests

if WIN32
SUBDIRS += win32/icons
endif

EXTRA_DIST= BUGS autogen gettext.h export.h
DISTCLEANFILES = _stdint.h


AM_CFLAGS = -Wall -I${srcdir}/sys -I${srcdir}/libtty -I${srcdir}/common
include sys/Makefile.am
include libstream/Makefile.am
include libtty/Makefile.am
include common/Makefile.am
include tmp/Makefile.am
include rec/Makefile.am
include play/Makefile.am
include proxy/Makefile.am
include time/Makefile.am
include cat/Makefile.am
