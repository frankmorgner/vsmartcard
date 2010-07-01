#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([picc_to_pcsc], [0.1], [oepen@informatik.hu-berlin.de])
AC_CONFIG_SRCDIR([src/picc_to_pcsc.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_PATH_PYTHON
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PKG_CHECK_EXISTS([libpcsclite],
                 [PKG_CHECK_MODULES([PCSC], [libpcsclite])],
                 [AC_MSG_WARN([libpcsclite not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $PCSC_CFLAGS"
LIBS="$LDFLAGS $PCSC_LIBS"
AC_CHECK_HEADERS(winscard.h,,
        [ AC_MSG_ERROR([winscard.h not found, install libpcsclite or use ./configure PCSC_CFLAGS=...]) ])
AC_MSG_CHECKING([for SCardEstablishContext])
AC_TRY_LINK_FUNC(SCardEstablishContext, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libpcsclite not found, use ./configure PCSC_LIBS=...]) ])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC


cat << EOF

picc_to_pcsc has been configured with following options:

Version:              ${PACKAGE_VERSION}
User binaries:        $(eval eval eval echo "${bindir}")
Configuration files:  $(eval eval eval echo "${sysconfdir}")


Host:                 ${host}
Compiler:             ${CC}
Preprocessor flags:   ${CPPFLAGS}
Compiler flags:       ${CFLAGS}
Preprocessor flags:   ${CPPFLAGS}
Linker flags:         ${LDFLAGS}
Libraries:            ${LIBS}
PCSC_CFLAGS:          ${PCSC_CFLAGS}
PCSC_LIBS:            ${PCSC_LIBS}

EOF


AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
