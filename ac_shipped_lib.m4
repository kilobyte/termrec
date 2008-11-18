dnl Check if our static copy of library works.

AC_DEFUN([AC_SHIPPED_LIB], [
AC_MSG_CHECKING([whether our shipped $2 works])
baselib=`echo "$1"|tr A-Z a-z`
ac_shipped_lib_save_LIBS=$LIBS
LIBS="-L$5lib -l${baselib#lib}"
AC_LINK_IFELSE([#include "$5lib/$3"
int main(){$4;return 0;}], [
  AC_DEFINE([SHIPPED_$1], [1], [Define if shipped $2 works.])
  AC_DEFINE([SHIPPED_$1_H], ["$5lib/$3"], [Set to your custom $3])
  $1="-L`pwd`/$5lib -l${baselib#lib}"
  AC_MSG_RESULT([yes])
], [AC_MSG_RESULT([no])])
LIBS=$ac_shipped_lib_save_LIBS
])

dnl Tell autoconf we have lib$1 but don't add it to LIBS.

AC_DEFUN([AC_HAVE_LIB], [
  LIB$2=-l$1
  AC_DEFINE([HAVE_LIB$2], [1], [Define to 1 if you have the `$1' library (-l$1).])
])
