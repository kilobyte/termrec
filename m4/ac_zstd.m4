dnl Ancient versions of zstd lack interfaces we use.

AC_DEFUN([AC_ZSTD], [
  AC_MSG_CHECKING([for working zstd])
  AC_COMPILE_IFELSE([AC_LANG_SOURCE([#include <zstd.h>
int main()
{
    size_t const inbufsz  = ZSTD_DStreamInSize();
    ZSTD_DStream* const stream = ZSTD_createDStream();
    ZSTD_freeDStream(stream);
    return inbufsz;
}
])], [AC_MSG_RESULT([yes])
      AC_DEFINE([WORKING_ZSTD], [1], [Set to 1 if new enough])], [
    AC_MSG_RESULT([no, missing or no good])
    ])
  ])
])
