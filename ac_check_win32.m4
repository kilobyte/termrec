dnl Check for WIN32, without needing all the config.guess bulkiness.

AC_DEFUN([AC_CHECK_WIN32], [
AC_CACHE_CHECK([for Win32], [ac_cv_is_win32], [
ac_check_win32_save_LIBS=$LIBS
LIBS=-lgdi32
AC_LINK_IFELSE([#include <windows.h>
int main(){TextOutW(0,0,0,0,0);return 0;}], [ac_cv_is_win32=yes], [ac_cv_is_win32=no])
LIBS=$ac_check_win32_save_LIBS
])
if test $ac_cv_is_win32 = yes; then
  AC_DEFINE([IS_WIN32], 1, [Define if you're a Bill worshipper.])
fi
])
