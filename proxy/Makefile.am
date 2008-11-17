bin_PROGRAMS += proxyrec

proxyrec_SOURCES = proxy/proxy.c
proxyrec_LDADD = libcommon.la libutils.la libtty.la @PTHREAD_LIBS@
proxyrec_LDFLAGS = @PTHREAD_CFLAGS@

if WIN32
proxyrec_LDADD += win32/icons/proxyrec.coff
proxyrec_LDFLAGS += -Xlinker --subsystem -Xlinker windows
endif
