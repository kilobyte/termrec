dnl can we block unwanted symbols?
AC_DEFUN([AC_VISIBILITY], [
AC_MSG_CHECKING([for GCC visibility attr])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
# define export __attribute__ ((visibility ("default")))
# pragma GCC visibility push(hidden)
])], [AC_DEFINE([GCC_VISIBILITY], [1], [Define to 1 if the compiler uses GCC's visibility __attribute__s.])
    AC_MSG_RESULT([yes])
], [AC_MSG_RESULT([no])])
])
