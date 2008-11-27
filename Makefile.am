SUBDIRS = tests

if WIN32
SUBDIRS += win32/icons
endif

EXTRA_DIST= BUGS autogen gettext.h export.h win32/lib/README
DISTCLEANFILES = _stdint.h

CLEANFILES =

AM_CFLAGS = -Wall -I${srcdir}/sys -I${srcdir}/libtty -I${srcdir}/common
include sys/Makefile.am
include libstream/Makefile.am
include libtty/Makefile.am
include common/Makefile.am
include proxy/Makefile.am
if WIN32
include win32/Makefile.am
else
include rec/Makefile.am
include play/Makefile.am
include time/Makefile.am
include cat/Makefile.am
endif
include doc/Makefile.am
