#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(termrec, 0.13.a, [kilobyte@angband.pl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([formats.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile])

# Checks for programs.
AC_PROG_CC

# Checks for Winblows.
AC_CHECK_WIN32
AC_CHECK_TOOL(WINDRES, windres, :)
AM_CONDITIONAL(WIN32, test x$ac_cv_is_win32 = xyes)
AC_LIB_WINSOCK2

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIOCGWINSZ
AC_CREATE_STDINT_H
AC_HEADER_TIME
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h strings.h sys/ioctl.h termios.h unistd.h bzlib.h zlib.h sys/time.h getopt.h sys/select.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h])
AC_DECL_OR_ZERO([O_BINARY], [fcntl.h])
AC_DECL_OR_ZERO([AI_ADDRCONFIG], [netdb.h])
AC_DECL_OR_ZERO([AI_NUMERICSERV], [netdb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN

AC_PTYRANGES

# Check for Posix threads.
ACX_PTHREAD

# Checks for library functions.
AC_CHECK_FUNCS([getpt ptsname snprintf vsnprintf _getpty grantpt forkpty asprintf gettimeofday usleep ctime_r getopt_long getaddrinfo])
AC_CHECK_LIB([bz2], [BZ2_bzBuffToBuffCompress], , [
  AC_SHIPPED_LIB(LIBBZ2, libbz2.a, bzlib.h, [BZ2_bzRead(0,0,0,0)], [win32/])])
AC_SUBST([LIBBZ2])
AC_CHECK_LIB([z], [gzread], , [
  AC_SHIPPED_LIB(LIBZ, libz.a, zlib.h, [gzread(0,0,0)], [win32/])])
AC_SUBST([LIBZ])
AC_OUTPUT
