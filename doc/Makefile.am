MANPAGES = libtty.3
dist_man_MANS = $(MANPAGES:%=doc/%)
EXTRA_DIST += $(dist_man_MANS:%.3=%.pod)

if POD
# If we're in a git repository, most likely the timestamp will be useless
# -- but we can check the last commit date.  When not in git, it's most
# likely a tarball with files with good timestamps.
# Since leaving the git world goes through "make dist", let's update the
# timestamps there -- oh lookie, it already depends on the built manpages.
.pod.3:
	d=`git log -1 --pretty="format:%ct" $<|cut -f 1 -d ' '`;\
	if test -n "$$d";then touch $< -d "@$$d";fi;\
	pod2man -c termrec -s 3 -n `basename "$@"|sed 's/\.[0-9]$$//'|tr a-z A-Z` -r "$(VERSION)" $< >$@

CLEANFILES += $(dist_man_MANS)
endif
