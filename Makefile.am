if WIN32
SUBDIRS = win32/icons . tests
else
SUBDIRS = . tests
endif

EXTRA_DIST= BUGS autogen.sh gettext.h export.h win32/termrec.iss \
	VERSION get_version
ACLOCAL_AMFLAGS = -I m4

CLEANFILES =

AM_CFLAGS = -I${srcdir}/sys -I${srcdir}/libtty -I${srcdir}/common -Wno-parentheses
include sys/Makefile.am
include libstream/Makefile.am
include libtty/Makefile.am
include common/Makefile.am
if WIN32
include win32/Makefile.am
else
include rec/Makefile.am
include play/Makefile.am
include time/Makefile.am
include cat/Makefile.am
endif
include doc/Makefile.am

VERSION: FORCE
	[ ! -d .git ] || $(srcdir)/get_version >VERSION
FORCE:
