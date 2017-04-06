#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([PC/SC Relay], [0.5], [https://github.com/frankmorgner/vsmartcard/issues], [pcsc-relay], [http://frankmorgner.github.io/vsmartcard/pcsc-relay/README.html])
AC_CONFIG_SRCDIR([src/pcsc-relay.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE(foreign)

dnl Check for windows build
case "${host}" in
    *-mingw*|*-winnt*)
        WIN32="yes"
        ;;
    *-cygwin*)
        WIN32="yes"
        ;;
esac
AM_CONDITIONAL([WIN32], [test "${WIN32}" = "yes"])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AM_PATH_PYTHON
AC_PROG_SED
PKG_PROG_PKG_CONFIG
AX_PTHREAD
AC_ARG_VAR([HELP2MAN],
           [absolute path to help2man used for man page generation of pcsc-relay])
AC_PATH_PROG(HELP2MAN, help2man, not found)
if test ! -r src/pcsc-relay.1 -a "${HELP2MAN}" = "not found"
then
    if test "${HELP2MAN}" = "not found"
    then
        AC_MSG_ERROR([Need help2man to generate man page for pcsc-relay])
    fi
else
    # make sure these files are the most recent
    touch src/pcsc-relay.1
fi
AC_ARG_VAR([GENGETOPT],
           [absolute path to gengetopt used for command line parsing of pcsc-relay])
AC_PATH_PROG(GENGETOPT, gengetopt, not found)
if test ! -r src/cmdline.h -o ! -r src/cmdline.c
then
    if test "${GENGETOPT}" = "not found"
    then
        AC_MSG_ERROR([Need gengetopt for parsing command line of pcsc-relay])
    fi
else
    # make sure these files are the most recent
    touch src/pcsc-relay.ggo
    touch src/cmdline.h src/cmdline.h
fi

# If you need to see the details, just run make V=1.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ARG_ENABLE(
	[libnfc],
	[AS_HELP_STRING([--enable-libnfc],[enable emulation hardware supported via libnfc @<:@detect@:>@])],
	,
	[enable_libnfc="detect"]
)

# Checks for libraries.
PKG_CHECK_EXISTS([libpcsclite],
                 [PKG_CHECK_MODULES([PCSC], [libpcsclite])],
                 [AC_MSG_WARN([libpcsclite not found by pkg-config])])

# Checks for libraries.
PKG_CHECK_EXISTS([libnfc >= 1.6],
                 [PKG_CHECK_MODULES([LIBNFC], [libnfc >= 1.6])])


saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"

CPPFLAGS="$CPPFLAGS $PCSC_CFLAGS"
LIBS="$LIBS $PCSC_LIBS"
AC_CHECK_HEADERS(winscard.h, [], [ AC_MSG_ERROR([winscard.h not found, install PC/SC Lite or similar or use ./configure PCSC_CFLAGS=...]) ])
AC_CHECK_HEADERS(reader.h, [AC_DEFINE(HAVE_READER_H, 1, [use reader.h from PC/SC Lite])], [])
AC_CHECK_HEADERS(wintypes.h, [AC_DEFINE(HAVE_WINTYPES_H, 1, [use wintypes.h from PC/SC Lite])], [])
AC_CHECK_HEADERS(pcsclite.h, [AC_DEFINE(HAVE_PCSCLITE_H, 1, [use pcsclite.h from PC/SC Lite])], [])
AC_MSG_CHECKING([for SCardEstablishContext])
AC_TRY_LINK_FUNC(SCardEstablishContext, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libpcsclite not found, use ./configure PCSC_LIBS=...]) ])

have_libnfc="yes"
CPPFLAGS="$saved_CPPFLAGS $LIBNFC_CFLAGS"
LIBS="$saved_LIBS $LIBNFC_LIBS"
AC_CHECK_HEADERS(nfc/nfc.h, , [have_libnfc="no"])
AC_MSG_CHECKING([for nfc_initiator_select_passive_target])
AC_TRY_LINK_FUNC(nfc_initiator_select_passive_target,
                 [AC_MSG_RESULT([yes])],
                 [AC_MSG_RESULT([no])
                  have_libnfc="no" ])

CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"

case "${enable_libnfc}" in
	no)
		have_libnfc="no"
	;;
	detect)
		if test "${have_libnfc}" = "yes"; then
			enable_libnfc="yes"
		else
			enable_libnfc="no"
		fi
	;;
esac

if test "${enable_libnfc}" = "yes"; then
	if test "${have_libnfc}" = "yes"; then
		AC_DEFINE([ENABLE_LIBNFC], [1], [Use emulation hardware supported via libnfc])
	else
		AC_MSG_ERROR([libnfc >= 1.6 not found, use ./configure LIBNFC_LIBS=... LIBNFC_CFLAGS=...])
	fi
fi
AM_CONDITIONAL([ENABLE_LIBNFC], [test "${enable_libnfc}" = "yes"])


# --enable-piccdev=DEV
AC_ARG_ENABLE(piccdev,
              AC_HELP_STRING([--enable-piccdev=DEV],[OpenPICC character device (default: /dev/ttyACM0)]),
              [piccdev="${enableval}"], [piccdev="/dev/ttyACM0"])
AC_DEFINE_UNQUOTED(PICCDEV, "${piccdev}", [OpenPICC character device])


PACKAGE_SUMMARY="Relay a smart card to a contactless interface"
AC_SUBST(PACKAGE_SUMMARY)


# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h unistd.h termios.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_CHECK_DECLS([SO_NOSIGPIPE], [], [], [#include <sys/socket.h>])
AC_CHECK_DECLS([MSG_NOSIGNAL], [], [], [#include <sys/socket.h>])

# Checks for library functions.
AC_CHECK_FUNCS([sigaction tcgetattr strerror strtol strtoul])
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC


cat << EOF

pcsc-relay has been configured with following options:

Version:              ${PACKAGE_VERSION}
User binaries:        $(eval eval eval echo "${bindir}")
Enable libnfc:        ${enable_libnfc}

Host:                 ${host}
Compiler:             ${CC}
Preprocessor flags:   ${CPPFLAGS}
Compiler flags:       ${CFLAGS}
Preprocessor flags:   ${CPPFLAGS}
Linker flags:         ${LDFLAGS}
Libraries:            ${LIBS}
PTHREAD_LIBS:         ${PTHREAD_LIBS}
PTHREAD_CFLAGS:       ${PTHREAD_CFLAGS}
PCSC_CFLAGS:          ${PCSC_CFLAGS}
PCSC_LIBS:            ${PCSC_LIBS}
LIBNFC_CFLAGS:        ${LIBNFC_CFLAGS}
LIBNFC_LIBS:          ${LIBNFC_LIBS}

OpenPICC device:      ${piccdev}


HELP2MAN:             ${HELP2MAN}
GENGETOPT:            ${GENGETOPT}

EOF


AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 win32/Makefile
                 src/Makefile])
AC_OUTPUT
