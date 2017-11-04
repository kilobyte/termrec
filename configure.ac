#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(termrec, 0.17, [kilobyte@angband.pl])
AM_INIT_AUTOMAKE([-Wall dist-xz no-dist-gzip foreign subdir-objects])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([libtty/tty.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES(Makefile
        win32/icons/Makefile
        )
# Use kernel-like CC instead of the usual verbose insanity.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:])
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])
LT_INIT([win32-dll])
AC_CHECK_PROG(POD2MAN, pod2man, yes, no)
AM_CONDITIONAL(POD, test x$ac_cv_prog_POD2MAN = xyes)
AC_VISIBILITY

# Use a modern C dialect.
AX_CHECK_COMPILE_FLAG(-std=gnu11, [CFLAGS="$CFLAGS -std=gnu11"],
 AX_CHECK_COMPILE_FLAG(-std=gnu99, [CFLAGS="$CFLAGS -std=gnu99"]))

# Check if anyone reads build logs.
AC_SCO

# Checks for header files.
AC_HEADER_STDC
#AC_HEADER_TIOCGWINSZ
AX_CREATE_STDINT_H
AC_HEADER_TIME
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h strings.h sys/ioctl.h termios.h \
        unistd.h bzlib.h zlib.h sys/time.h sys/select.h langinfo.h \
        netinet/in.h lzma.h pty.h libutil.h zstd.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h sys/socket.h])
AC_CHECK_HEADERS([winsock2.h])
AC_DECL_OR_ZERO([O_BINARY], [fcntl.h])
AC_DECL_OR_ZERO([AI_ADDRCONFIG], [netdb.h])
AC_DECL_OR_ZERO([AI_NUMERICSERV], [netdb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN

AC_PTYRANGES
AC_DEV_PTMX

# Check for Posix threads.
AX_PTHREAD

# Checks for library functions.
AC_SEARCH_LIBS(forkpty, util)
AC_SEARCH_LIBS(clock_gettime, rt)
AC_CHECK_FUNCS([getpt ptsname snprintf vsnprintf _getpty grantpt forkpty asprintf \
         clock_gettime nanosleep ctime_r getopt_long getaddrinfo fork pipe select \
         posix_openpt])
AM_CONDITIONAL(HAVE_FORK, test x$ac_cv_func_fork = xyes)

# Checks for Winblows.
AC_CHECK_WIN32
AC_CHECK_TOOL(WINDRES, windres, :)
AM_CONDITIONAL(WIN32, test x$ac_cv_is_win32 = xyes)
AM_CONDITIONAL(WIN32NET, test x$ac_cv_is_win32 = xyes -a x$ac_cv_func_getaddrinfo = xno)
AC_LIB_WINSOCK2
AC_FUNC_WIN32_GAI_STERRROR
AC_FUNC_WIN32_GETADDRINFO
AX_LIB_SOCKET_NSL
AC_CHECK_LIB([bz2], [BZ2_bzBuffToBuffCompress], [AC_HAVE_LIB([bz2], [BZ2])], [
  AC_SHIPPED_LIB(LIBBZ2, bzip2.dll, bzlib.h, [BZ2_bzRead(0,0,0,0)], [win32/])])
AC_SUBST([LIBBZ2])
AC_CHECK_LIB([z], [gzread], [AC_HAVE_LIB([z], [Z])], [
  AC_SHIPPED_LIB(LIBZ, zlib1.dll, zlib.h, [gzread(0,0,0)], [win32/])])
AC_SUBST([LIBZ])
AC_CHECK_LIB([lzma], [lzma_code], [AC_HAVE_LIB([lzma], [LZMA])], [
  AC_SHIPPED_LIB(LIBLZMA, lzma.dll, lzma.h, [lzma_code(0,0)], [win32/])])
AC_SUBST([LIBLZMA])
AC_CHECK_LIB([zstd], [ZSTD_compress], [AC_HAVE_LIB([zstd], [ZSTD])], [
  AC_SHIPPED_LIB(LIBZSTD, zstd.dll, zstd.h, [ZSTD_compress(0,0,0,0,0)], [win32/])])
AC_SUBST([LIBZSTD])

# Enable warnings.
AC_C_TRY_FLAG([-Wall])
AC_C_TRY_FLAG([-Wwrite-strings])
AC_C_TRY_FLAG([-Wstrict-prototypes])
AC_C_TRY_FLAG([-Wshadow])
AC_C_TRY_FLAG([-Wmissing-prototypes])
AC_C_TRY_FLAG([-Wno-unused-parameter])
AC_C_TRY_FLAG([-Wno-bitwise-op-parentheses])

# Initialize the test suite.
AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([tests/Makefile tests/atlocal])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_OUTPUT
