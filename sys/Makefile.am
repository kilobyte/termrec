noinst_LTLIBRARIES = libcompat.la
libcompat_la_SOURCES = asprintf.c

if WIN32
libcompat_la_SOURCES += win32/net.c win32/winutils.c
else
libcompat_la_SOURCES += unix/utils.c
endif
