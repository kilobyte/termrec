dnl Compiler (and not headers/libs/etc)-specific stuff.

dnl Is -Wall supported?
AC_DEFUN([AC_WALL], [
AC_MSG_CHECKING([if -Wall works])
if $CC -Wall -c /dev/null /dev/null >/dev/null 2>/dev/null
  then
    WALL="-Wall"
    AC_MSG_RESULT([yes])
  else
    WALL=""
    AC_MSG_RESULT([no])
fi
AC_SUBST([WALL])
])

dnl can we block unwanted symbols?
AC_DEFUN([AC_VISIBILITY], [
AC_MSG_CHECKING([aaa])
AC_COMPILE_IFELSE([
# define export __attribute__ ((visibility ("default")))
# pragma GCC visibility push(hidden)
], [AC_DEFINE([GCC_VISIBILITY], [1], [Define to 1 if the compiler uses GCC's visibility __attribute__s.])
    AC_MSG_RESULT([yes])
], [AC_MSG_RESULT([no])])
])
