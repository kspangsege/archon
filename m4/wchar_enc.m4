AC_DEFUN([ARCHON_CHECK_WCHAR_ENC], [

  AC_SUBST([ARCHON_WCHAR_ENC_IS_UCS], [0])

  AC_MSG_CHECKING(which encoding is used for wchar_t)
  AC_PREPROC_IFELSE([AC_LANG_SOURCE([
#include <stdlib.h>
#if !(defined(__STDC_ISO_10646__) || defined(__APPLE__))
#error
#endif
  ])], [
    ARCHON_WCHAR_ENC_IS_UCS=1
    AC_MSG_RESULT([UCS])
  ], [
    AC_MSG_ERROR([Failed], [1])
  ])
])
