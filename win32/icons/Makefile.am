all-local: termplay.coff termrec.coff

termplay_RES = termplay.rc termplay.ico termplay.manifest open.bmp rewind.bmp pause.bmp play.bmp wood1.bmp selstart.bmp selend.bmp export.bmp
termrec_RES = termrec.rc termrec.ico
EXTRA_DIST= ${termplay_RES} ${termrec_RES}

CLEANFILES = termplay.coff termrec.coff

termplay.coff: ${termplay_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termplay.rc "$$builddir/termplay.coff"
termrec.coff: ${termrec_RES}
	builddir=`pwd` && cd $(srcdir) && \
	  $(WINDRES) termrec.rc "$$builddir/termrec.coff"
