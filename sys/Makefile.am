noinst_LTLIBRARIES = libcompat.la
libcompat_la_SOURCES = asprintf.c is_utf8.c threads.h compat.h utils.h error.h
AM_CFLAGS = -Wall

if WIN32
libcompat_la_SOURCES += win32/net.c win32/winutils.c win32/net.h win32/threads.h win32/winutils.h
else
libcompat_la_SOURCES += unix/utils.c unix/threads.h
endif
