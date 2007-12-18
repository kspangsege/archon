#
#
#  Check if SAMBA building uses abbreviated output form.
#
#

#
#
#  MUST CHEACK FOR CORBA (lib) ORB AND IDL-COMPILER (prog) (Need it be
#  OmniORB?)
#
#

#
#
# How can I add -Wold-style-cast generally and then disable in on
# specific file compilations?
#
#

#
# Considder the use of 'pkg-config' when available.
#

#
# Currently IDL output files are not cleaned, so an upgrade of omniORB results in errors. These files should be removed by a 'distclean'. They should not be removed by plain 'clean' since then the tarball will require omniORB to build.
# considder using http://autotools-idl.sourceforge.net/
#

# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)
AC_INIT([Archon],[0.4],[kristian.spangsege@gmail.com])
AC_REVISION([$Revision$])
AC_CONFIG_SRCDIR([src/archon/core/Makefile.am])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/archon/config.h])

AM_INIT_AUTOMAKE([nostdinc])

AX_PTHREAD

# Default set of CFLAGS (override this with ./configure CFLAGS=foo)
CFLAGS=${CFLAGS:=-Wall -Wextra -Werror -Wno-parentheses -Wnon-virtual-dtor -pedantic -ggdb -O0}
CXXFLAGS=${CXXFLAGS:=-Wall -Wextra -Werror -Wno-parentheses -Wnon-virtual-dtor -pedantic -ggdb -O0}
#CFLAGS=${CFLAGS:=-O3 -DNDEBUG -DARCHON_NDEBUG}
#CXXFLAGS=${CXXFLAGS:=-O3 -DNDEBUG -DARCHON_NDEBUG}

AX_CXX_COMPILE_STDCXX_14([noext])

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"


# Checks for programs.
AM_PROG_AR
AC_PROG_MAKE_SET
AC_PROG_CXX
AC_PROG_INSTALL

LT_INIT
#LT_LANG([C++])

# archon/core/
ARCHON_CHECK_WCHAR_ENC
ARCHON_CHECK_LIBREALTIME
ARCHON_CHECK_GET_TERM_SIZE

# archon/core/dir_scan.cpp
AC_CHECK_FUNCS([fpathconf dirfd])

# archon/util/
ARCHON_CHECK_LIBREADLINE
ARCHON_CHECK_LIBMAGIC
ARCHON_CHECK_LINUX_INOTIFY

# archon/image/
ARCHON_CHECK_IMAGE_LIBS

# archon/font/
ARCHON_CHECK_FONT_LIBS

# archon/dom/impl/
ARCHON_CHECK_LIBEXPAT
ARCHON_CHECK_SCRIPT_LIBS

# archon/display/
ARCHON_CHECK_DISPLAY_LIBS

# archon/web/
ARCHON_CHECK_WEB_LIBS


AC_CONFIG_FILES([src/archon/core/Makefile
                 src/archon/core/test/Makefile
                 src/archon/math/Makefile
                 src/archon/math/test/Makefile
                 src/archon/thread/Makefile
                 src/archon/thread/test/Makefile
                 src/archon/util/Makefile
                 src/archon/util/test/Makefile
                 src/archon/parser/Makefile
                 src/archon/parser/test/Makefile
                 src/archon/image/Makefile
                 src/archon/image/test/Makefile
                 src/archon/font/Makefile
                 src/archon/font/test/Makefile
                 src/archon/dom/Makefile
                 src/archon/dom/impl/Makefile
                 src/archon/dom/test/Makefile
                 src/archon/graphics/Makefile
                 src/archon/display/Makefile
                 src/archon/display/test/Makefile
                 src/archon/display/x11/Makefile
                 src/archon/render/Makefile
                 src/archon/render/test/Makefile
                 src/archon/web/Makefile
                 src/archon/web/test/Makefile
                 src/archon/web/server/Makefile
                 src/archon/web/server/test/Makefile
                 src/archon/raytrace/Makefile
                 src/archon/raytrace/test/Makefile
                 src/archon/Makefile
                 src/archon/config_pub.h
                 src/Makefile
                 Makefile])

AC_OUTPUT
