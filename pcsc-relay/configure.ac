#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([pcsc-relay], [0.2], [http://sourceforge.net/projects/vsmartcard/support])
AC_CONFIG_SRCDIR([src/pcsc-relay.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_PATH_PYTHON
PKG_PROG_PKG_CONFIG

# If you need to see the details, just run make V=1.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for libraries.
PKG_CHECK_EXISTS([libpcsclite],
                 [PKG_CHECK_MODULES([PCSC], [libpcsclite])],
                 [AC_MSG_WARN([libpcsclite not found by pkg-config])])
# Checks for libraries.
PKG_CHECK_EXISTS([libnfc >= 1.3.9],
                 [PKG_CHECK_MODULES([LIBNFC], [libnfc >= 1.3.9])],
                 [AC_MSG_WARN([libnfc >= 1.3.9 not found by pkg-config])])


saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $PCSC_CFLAGS $LIBNFC_CFLAGS"
LIBS="$LDFLAGS $PCSC_LIBS $LIBNFC_LIBS"
AC_CHECK_HEADERS(winscard.h, [], [ AC_MSG_ERROR([winscard.h not found, install PC/SC Lite or similar or use ./configure PCSC_CFLAGS=...]) ])
AC_CHECK_HEADERS(reader.h, [AC_DEFINE(HAVE_READER_H, 1, [use reader.h from PC/SC Lite])], [])
AC_CHECK_HEADERS(pcsclite.h, [AC_DEFINE(HAVE_PCSCLITE_H, 1, [use pcsclite.h from PC/SC Lite])], [])
AC_MSG_CHECKING([for SCardEstablishContext])
AC_TRY_LINK_FUNC(SCardEstablishContext, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libpcsclite not found, use ./configure PCSC_LIBS=...]) ])
AC_CHECK_HEADERS(nfc/nfc.h,,
        [ AC_MSG_ERROR([nfc/nfc.h not found, install libnfc >= 1.3.9 or use ./configure LIBNFC_CFLAGS=...]) ])
AC_MSG_CHECKING([for nfc_initiator_select_passive_target])
AC_TRY_LINK_FUNC(nfc_initiator_select_passive_target, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libnfc >= 1.3.9 not found, use ./configure LIBNFC_LIBS=...]) ])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"


# --enable-piccdev=DEV
piccdev_default=
AC_ARG_ENABLE(piccdev,
              AC_HELP_STRING([--enable-piccdev=DEV],[OpenPICC character device (default: /dev/ttyACM0)]),
              [piccdev="${enableval}"], [piccdev="/dev/ttyACM0"])
AC_DEFINE_UNQUOTED(PICCDEV, "${piccdev}", [OpenPICC character device])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC


cat << EOF

pcsc-relay has been configured with following options:

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
LIBNFC_CFLAGS:        ${LIBNFC_CFLAGS}
LIBNFC_LIBS:          ${LIBNFC_LIBS}

EOF


AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
