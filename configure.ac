#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xgraph], [12.1], [])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/alloc.c])
AC_CONFIG_HEADERS([autoconf.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# Check for X headers and libraries
AC_PATH_X

if test "x$no_x" = "xyes"; then
	AC_MSG_ERROR([xgraph requires the X Window System libraries and headers.])
fi

if test "x$x_includes" != "x"; then
	CFLAGS="$CFLAGS -I$x_includes"
fi
if test "x$x_libraries" != "x"; then
	LIBS="$LIBS -L$x_libraries"
fi

# Check for required functions in -lX11
XLIB=
AC_CHECK_LIB([X11], [XOpenDisplay], [XLIB=yes], [XLIB=no])
test "x$XLIB" = "xno" && AC_MSG_ERROR([standard X11 libraries not found])

LIBS="$LIBS -lX11"
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [floor])

# Checks for header files.
AC_CHECK_HEADERS([float.h limits.h memory.h stdlib.h string.h strings.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor memset pow sqrt strcasecmp strchr])

AC_CONFIG_FILES([Makefile
				 src/Makefile])
AC_OUTPUT
