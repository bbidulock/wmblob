
#
#   this file is part of: wmblob
#
#   the GNU GPL license applies to this file, see file COPYING for details.
#   author: jean-luc herren.
#

AC_PREREQ(2.57)
AC_INIT(wmblob, 1.0.4, [jlh-at-gmx-dot-ch])

AC_MSG_RESULT([
        +-----------------------------------+
        |  thank you for installing wmblob  |
        +-----------------------------------+
])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([dist-bzip2])

AC_CONFIG_SRCDIR([src/wmblob.c])
AC_CONFIG_HEADER([src/config.h])

# more flags are added later.  this is mainly to remove -g
CFLAGS=""

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# check X paths
AC_PATH_X
LIBS="$LIBS -L$x_libraries"

# Checks for libraries.
AC_CHECK_LIB([X11], [XOpenDisplay], [],
	[AC_MSG_ERROR([**** could not find libX11])])
AC_CHECK_LIB([Xext], [XShapeCombineMask], [],
	[AC_MSG_ERROR([**** could not find libXext])])
AC_CHECK_LIB([Xpm], [XpmCreatePixmapFromData], [],
	[AC_MSG_ERROR([**** could not find libXpm])])
AC_CHECK_LIB([m], [atan2], [],
	[AC_MSG_ERROR([**** could not find libm])])

# add them all in one go...
LIBS="$LIBS -lX11 -lXext -lXpm -lm"

# check for pkg-config

# we do it ourself, because AC_PATH_PROG does not work for me (?)
AC_MSG_CHECKING([for pkg-config])
PKGCONFIG=no
pkg-config --version > /dev/null 2>&1 && PKGCONFIG=yes
AC_MSG_RESULT([$PKGCONFIG])

if test "$PKGCONFIG" = no; then
	AC_MSG_ERROR([**** cannot find pkg-config.])
fi

# check for gtk+ 2.0

AC_MSG_CHECKING([for GTK+-2.0])

if pkg-config gtk+-2.0 > /dev/null 2>&1; then
	AC_MSG_RESULT([yes])
	LIBS="$LIBS `pkg-config --libs gtk+-2.0`"
	CFLAGS="$CFLAGS `pkg-config --cflags gtk+-2.0`"
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([**** could not find GTK+-2.0])
fi


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h stdlib.h string.h unistd.h])
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strchr strdup uname])

# compiler flags
CFLAGS="$CFLAGS -O2 -Wall"

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	doc/Makefile
])

AC_OUTPUT

AC_MSG_RESULT([
        +-------------------------------------+
        |  configuration done.  type `make'.  |
        |  and then `make install'.           |
        +-------------------------------------+
])

