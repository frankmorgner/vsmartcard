#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT(pinpad, 0.1, oepen@informatik.hu-berlin.de)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PATH_PYTHON

# Checks for libraries.
PKG_CHECK_MODULES(PYGTK, pygtk-2.0)
AC_SUBST(PYGTK_CFLAGS)
AC_SUBST(PYGTK_LIBS)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
