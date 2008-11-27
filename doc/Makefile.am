MANPAGES = libtty.3
dist_man_MANS = $(MANPAGES:%=doc/%)
EXTRA_DIST += $(dist_man_MANS:%.3=%.pod)

if POD
.pod.3:
	pod2man -c termrec -s 3 -n `basename "$@"|sed 's/\.[0-9]$$//'|tr a-z A-Z` -r "$(VERSION)" $< >$@
CLEANFILES += $(dist_man_MANS)
endif
