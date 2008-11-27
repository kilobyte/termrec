all-local: termplay.coff termrec.coff proxyrec.coff

termplay_RES = termplay.rc termplay.ico termplay.manifest open.bmp rewind.bmp pause.bmp play.bmp wood1.bmp selstart.bmp selend.bmp export.bmp
termrec_RES = termrec.rc termrec.ico
proxyrec_RES = proxyrec.rc proxyrec.ico
EXTRA_DIST= ${termplay_RES} ${termrec_RES} ${proxyrec_RES}

CLEANFILES = termplay.coff termrec.coff proxyrec.coff

termplay.coff: ${termplay_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
termrec.coff: ${termrec_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termrec.rc "$$builddir/termrec.coff"
proxyrec.coff: ${proxyrec_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) proxyrec.rc "$$builddir/proxyrec.coff"
