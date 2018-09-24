check_PROGRAMS = seeks tarith loadsave rec vt vtmir
AM_CFLAGS = -I$(top_srcdir) -I$(top_srcdir)/libtty/
LDADD = -L.. -ltty ../libutils.la
vtmir_LDFLAGS = @PTHREAD_CFLAGS@
vtmir_LDADD = $(LDADD) @PTHREAD_LIBS@
vt_LDADD = $(LDADD) ../libtty/wcwidth.o

EXTRA_DIST = $(TESTSUITE_AT) testsuite.at testsuite package.m4 unescape
DISTCLEANFILES       = atconfig $(check_SCRIPTS)
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE) package.m4

dist-hook:
	for x in vt.in vt.out rec.in rec.out; do mkdir -p $(distdir)/$$x; \
	  cp -p $(srcdir)/$$x/* $(distdir)/$$x; done

## ------------ ##
## package.m4.  ##
## ------------ ##

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	{				      \
	  echo '# Signature of the current package.'; \
	  echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])'; \
	  echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])'; \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	  echo 'm4_define([AT_TESTS], [$(TESTSUITE_AT)])'; \
	} >$(srcdir)/package.m4

#

TESTSUITE = $(srcdir)/testsuite
check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) $(TESTSUITE)

clean-local:
	[ ! -x $(TESTSUITE) ] || $(SHELL) $(TESTSUITE) --clean

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 testsuite.at $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) $@.at -o $@.tmp
	mv $@.tmp $@

TESTSUITE_AT =
