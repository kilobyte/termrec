noinst_LTLIBRARIES = libcompat.la libutils.la
libcompat_la_SOURCES = asprintf.c threads.h compat.h
libutils_la_SOURCES = is_utf8.c utils.h error.h
AM_CFLAGS = -Wall

libutils_la_LIBADD = libcompat.la

if WIN32
libcompat_la_SOURCES += win32/net.c win32/net.h win32/threads.h
libutils_la_SOURCES += win32/winutils.c win32/winutils.h
else
libcompat_la_SOURCES += unix/threads.h
libutils_la_SOURCES += unix/utils.c
endif
