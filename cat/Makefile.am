bin_PROGRAMS = termcat
AM_CFLAGS = -Wall -I${srcdir}/../sys -I${srcdir}/../libtty -I${srcdir}/../common

termcat_SOURCES = termcat.c
termcat_LDADD = ../sys/libutils.la -L../libtty -ltty
