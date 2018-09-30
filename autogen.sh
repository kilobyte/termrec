#!/bin/sh
set -e

if which glibtoolize >/dev/null
  then glibtoolize --automake
  else libtoolize --automake
fi
aclocal
autoheader
automake --add-missing
autoconf
