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

dnl Add WinSock if it's present.

AC_DEFUN([AC_LIB_WINSOCK2], [
AC_MSG_CHECKING([for WinSock2])
ac_lib_ws2_save_LIBS=$LIBS
LIBS="-lws2_32"
AC_LINK_IFELSE([
#include "windows.h"
int main(){WSACleanup();return 0;}], [
  LIBS="$ac_lib_ws2_save_LIBS -lws2_32"
  AC_MSG_RESULT([yes])
], [
  LIBS=$ac_lib_ws2_save_LIBS
  AC_MSG_RESULT([no])
])
])

dnl Check if gai_strerror is #defined (like in new MINGW headers).

AC_DEFUN([AC_FUNC_GAI_STERRROR], [
AC_MSG_CHECKING([for gai_strerror])
AC_LINK_IFELSE([
#include <winsock2.h>
#include <windows.h> 
#include <ws2tcpip.h>
int main(){gai_strerror(0);return 0;}], [
  AC_DEFINE([HAVE_GAI_STRERROR], [1], [Define if present])
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
])
])
