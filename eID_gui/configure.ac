#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT(eid_gui, 0.2, oepen@informatik.hu-berlin.de)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PATH_PYTHON

# If you need to see the details, just run make V=1.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for libraries.
PKG_CHECK_MODULES(PYGTK, pygtk-2.0)
AC_SUBST(PYGTK_CFLAGS)
AC_SUBST(PYGTK_LIBS)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/pace-gui.desktop
                 src/change-pin.desktop
                 src/eid/Makefile
                 src/eid/eid_gui_globals.py
                 images/Makefile])
AC_OUTPUT
