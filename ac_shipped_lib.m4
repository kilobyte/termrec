dnl Check if our static copy of library works.

AC_DEFUN([AC_SHIPPED_LIB], [
AC_MSG_CHECKING([whether our shipped $2 works])
ac_shipped_lib_save_LIBS=$LIBS
LIBS="lib/$2"
AC_LINK_IFELSE([#include "lib/$3"
int main(){$4;return 0;}], [
  AC_DEFINE([SHIPPED_$1], [1], [Define if shipped $2 works.])
  $1="lib/$2"
  AC_MSG_RESULT([yes])
], [AC_MSG_RESULT([no])])
LIBS=$ac_shipped_lib_save_LIBS
])
