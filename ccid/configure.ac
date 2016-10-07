#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([USB CCID Emulator], [0.8], [https://github.com/frankmorgner/vsmartcard/issues], [ccid-emulator], [http://frankmorgner.github.io/vsmartcard/ccid/README.html])
AC_CONFIG_SRCDIR([src/ccid.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE(foreign)
AC_USE_SYSTEM_EXTENSIONS
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
AC_PROG_SED
PKG_PROG_PKG_CONFIG

AC_ARG_VAR([HELP2MAN],
           [absolute path to help2man used for man page generation of ccid-emulator])
AC_PATH_PROG(HELP2MAN, help2man, not found)
if test ! -r src/ccid-emulator.1 -a "${HELP2MAN}" = "not found"
then
    AC_MSG_ERROR([Need help2man to generate man page for ccid-emulator])
fi
AC_ARG_VAR([GENGETOPT],
           [absolute path to gengetopt used for command line parsing of ccid-emulator])
AC_PATH_PROG(GENGETOPT, gengetopt, not found)
if test ! -r src/cmdline.h -a "${GENGETOPT}" = "not found"
then
    AC_MSG_ERROR([Need gengetopt for parsing command line of ccid-emulator])
fi

# If you need to see the details, just run make V=1.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for libraries.

AC_CHECK_HEADERS(linux/usb/gadgetfs.h,,
                 [ AC_MSG_ERROR([linux/usb/gadgetfs.h not found, maybe you want to disable ccid]) ])

ACX_PTHREAD

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $PTHREAD_CFLAGS"
LIBS="$LDFLAGS $PTHREAD_LIBS"

AC_CHECK_HEADERS(pthread.h, [],
                 [ AC_MSG_ERROR([pthread.h not found, use ./configure PTHREAD_CFLAGS=... or disable ccid]) ])
AC_MSG_CHECKING([for pthread_create])
AC_TRY_LINK_FUNC(pthread_create, [ AC_MSG_RESULT([yes]) ],
                 [ AC_MSG_ERROR([pthread not found, use ./configure PTHREAD_LIBS=... or disable ccid]) ])

CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"


PKG_CHECK_EXISTS([libcrypto],
                 [PKG_CHECK_MODULES([OPENSSL], [libcrypto >= 1.0.0])],
                 [AC_MSG_WARN([libcrypto not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENSSL_CFLAGS"
LIBS="$LDFLAGS $OPENSSL_LIBS"
AC_CHECK_HEADERS(openssl/evp.h, [], [ AC_MSG_ERROR([openssl/evp.h not found, install OpenSSL >= 1.0.0 or use ./configure OPENSSL_CFLAGS=...]) ])
AC_MSG_CHECKING([for EVP_read_pw_string_min])
AC_TRY_LINK_FUNC(EVP_read_pw_string_min, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([OpenSSL >= 1.0.0 not found, use ./configure OPENSSL_LIBS=...]) ])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
PKG_CHECK_EXISTS([libopensc],
                 [PKG_CHECK_MODULES([OPENSC], [libopensc >= 0.12.1])],
                 [AC_MSG_WARN([libopensc >= 0.12.1 not found by pkg-config])])
test -z "$OPENSC_LIBS" && OPENSC_LIBS="-lopensc $OPENSSL_LIBS"
LIBS="$LDFLAGS $OPENSC_LIBS"
AC_MSG_CHECKING([for sc_bytes2apdu])
AC_TRY_LINK_FUNC(sc_bytes2apdu, [ AC_MSG_RESULT([yes]) ],
[ AC_MSG_ERROR([libopensc >= 0.12.1 not found, use ./configure OPENSC_LIBS=...]) ])
AC_CHECK_FUNCS([sc_apdu_set_resp sc_apdu_get_octets _sc_match_atr boxing_buf_to_pace_input])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"


PACKAGE_SUMMARY="Emulate a USB CCID compliant smart card reader"
AC_SUBST(PACKAGE_SUMMARY)


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h memory.h stdint.h stdlib.h string.h sys/ioctl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([memmove memset strerror strtol])

cat << EOF

${PACKAGE} has been configured with following options:

Version:              ${PACKAGE_VERSION}
User binaries:        $(eval eval eval echo "${bindir}")
Libraries:            $(eval eval eval echo "${libdir}")
Configuration files:  $(eval eval eval echo "${sysconfdir}")


Host:                 ${host}
Compiler:             ${CC}
Preprocessor flags:   ${CPPFLAGS}
Compiler flags:       ${CFLAGS}
Preprocessor flags:   ${CPPFLAGS}
Linker flags:         ${LDFLAGS}
Libraries:            ${LIBS}
PTHREAD_CFLAGS:       ${PTHREAD_CFLAGS}
PTHREAD_LIBS:         ${PTHREAD_LIBS}
OPENSC_LIBS:          ${OPENSC_LIBS}
OPENSSL_CFLAGS:       ${OPENSSL_CFLAGS}
OPENSSL_LIBS:         ${OPENSSL_LIBS}


HELP2MAN:             ${HELP2MAN}
GENGETOPT:            ${GENGETOPT}

EOF

AC_CONFIG_FILES([
                 Makefile
                 m4/Makefile
                 patches/Makefile
                 src/Makefile
                 doc/Makefile
                 ])
AC_OUTPUT
