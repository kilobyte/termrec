bin_PROGRAMS = termtime
AM_CFLAGS = -Wall -I${srcdir}/../sys -I${srcdir}/../libtty -I${srcdir}/../common

termtime_SOURCES = termtime.c
termtime_LDADD = ../sys/libutils.la ../common/libcommon.la -L../libtty -ltty
