#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(termrec, 0.16, [kilobyte@angband.pl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([libtty/vt100.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES(Makefile
	win32/icons/Makefile
	)
# Use kernel-like CC instead of the usual verbose insanity.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:])
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_CHECK_PROG(POD2MAN, pod2man, yes, no)
AM_CONDITIONAL(POD, test x$ac_cv_prog_POD2MAN = xyes)
AC_WALL
AC_VISIBILITY

# Checks for Winblows.
AC_CHECK_WIN32
AC_CHECK_TOOL(WINDRES, windres, :)
AM_CONDITIONAL(WIN32, test x$ac_cv_is_win32 = xyes)
AC_LIB_WINSOCK2

# Check if anyone reads build logs.
AC_SCO

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_TIOCGWINSZ
AC_CREATE_STDINT_H
AC_HEADER_TIME
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h strings.h sys/ioctl.h termios.h \
	unistd.h bzlib.h zlib.h sys/time.h getopt.h sys/select.h langinfo.h \
	netinet/in.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h sys/socket.h])
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
AC_CHECK_FUNCS([getpt ptsname snprintf vsnprintf _getpty grantpt forkpty asprintf \
	 gettimeofday usleep ctime_r getopt_long getaddrinfo fork pipe select])
AM_CONDITIONAL(HAVE_FORK, test x$ac_cv_func_fork = xyes)
AC_FUNC_GAI_STERRROR
AC_CHECK_LIB([bz2], [BZ2_bzBuffToBuffCompress], [AC_HAVE_LIB([bz2], [BZ2])], [
  AC_SHIPPED_LIB(LIBBZ2, bzip2.dll, bzlib.h, [BZ2_bzRead(0,0,0,0)], [win32/])])
AC_SUBST([LIBBZ2])
AC_CHECK_LIB([z], [gzread], [AC_HAVE_LIB([z], [Z])], [
  AC_SHIPPED_LIB(LIBZ, zlib1.dll, zlib.h, [gzread(0,0,0)], [win32/])])
AC_SUBST([LIBZ])

# Initialize the test suite.
AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([tests/Makefile tests/atlocal])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_OUTPUT
