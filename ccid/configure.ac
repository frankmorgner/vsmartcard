#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([ccid-emulator], [0.3], [http://sourceforge.net/projects/vsmartcard/support])
AC_CONFIG_SRCDIR([src/ccid.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
LT_INIT

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PKG_CHECK_EXISTS([libopensc],
                 [PKG_CHECK_MODULES([OPENSC], [libopensc < 0.12])],
                 [AC_MSG_WARN([libopensc < 0.12 not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENSC_CFLAGS"
LIBS="$LDFLAGS $OPENSC_LIBS"
AC_CHECK_HEADERS(opensc/opensc.h,,
        [ AC_MSG_ERROR([opensc/opensc.h not found, install libopensc < 0.12 or use ./configure OPENSC_CFLAGS=...]) ])
AC_MSG_CHECKING([for sc_context_create])
AC_TRY_LINK_FUNC(sc_context_create, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libopensc < 0.12 not found, use ./configure OPENSC_LIBS=...]) ])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"

# --disable-ccid
AC_ARG_ENABLE(ccid,
              AS_HELP_STRING([--disable-ccid], [Disable USB CCID emulator]),
              [enable_ccid="${enableval}"], [enable_ccid=yes])
if test "x$enable_ccid" != xno ; then
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

    enable_ccid=yes
fi
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)
AM_CONDITIONAL(WITH_CCID, test "${enable_ccid}" != "no")


PKG_CHECK_EXISTS([libssl],
                 [PKG_CHECK_MODULES([OPENSSL], [libssl])],
                 [AC_MSG_WARN([libssl not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENSSL_CFLAGS"
LIBS="$LDFLAGS $OPENSSL_LIBS"
AC_CHECK_HEADERS(openssl/evp.h, [], [ AC_MSG_ERROR([openssl/evp.h not found, install OpenSSL or use ./configure OPENSSL_CFLAGS=...]) ])
AC_MSG_CHECKING([for EVP_read_pw_string_min])
AC_TRY_LINK_FUNC(EVP_read_pw_string_min, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([OpenSSL not found, use ./configure OPENSSL_LIBS=...]) ])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"
AC_SUBST(OPENSSL_CFLAGS)
AC_SUBST(OPENSSL_LIBS)

# --enable-pace
AC_ARG_ENABLE(pace,
              AS_HELP_STRING([--enable-pace], [Enable Password Authenticated Connection Establishment (PACE)]),
              [enable_pace="${enableval}"], [enable_pace=no])
if test "x$enable_pace" != xno ; then
    PKG_CHECK_EXISTS([libpace],
                     [PKG_CHECK_MODULES([LIBPACE], [libpace])],
                     [AC_MSG_WARN([libpace not found by pkg-config])])
    saved_CPPFLAGS="$CPPFLAGS"
    saved_LIBS="$LIBS"
    CPPFLAGS="$CPPFLAGS $LIBPACE_CFLAGS"
    LIBS="$LDFLAGS $LIBPACE_LIBS"
    AC_CHECK_HEADERS(pace/pace.h, [], [ AC_MSG_ERROR([pace/pace.h not found, install libpace or use ./configure LIBPACE_CFLAGS=...]) ])
    AC_MSG_CHECKING([for EstablishPACEChannel])
    AC_TRY_LINK_FUNC(EstablishPACEChannel, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([libpace not found, use ./configure LIBPACE_LIBS=...]) ])
    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"

    enable_pace=yes
fi

AM_CONDITIONAL(WITH_PACE, test "${enable_pace}" != "no")
AM_COND_IF(WITH_PACE, [AC_DEFINE(WITH_PACE, 1, [enable PACE support])])
AM_COND_IF(WITH_PACE, [AC_DEFINE(BUERGERCLIENT_WORKAROUND, 1, [Always get EF.CardAccess, when connecting to a smart card. This is a workaround for the recent Buergerclient])])


# --disable-cats
AC_ARG_ENABLE(cats,
              AS_HELP_STRING([--enable-cats], [Disable tool for CAT-S and CAT-C (pinpad readers with PACE support)]),
              [enable_cats="${enableval}"], [enable_cats=no])
if test "x$enable_cats" != xno ; then
    PKG_CHECK_EXISTS([libpcsclite > 1.4.102],
                     [PKG_CHECK_MODULES([PCSC], [libpcsclite > 1.4.102])],
                     [AC_MSG_WARN([libpcsclite > 1.4.102 not found by pkg-config])])

    saved_CPPFLAGS="$CPPFLAGS"
    saved_LIBS="$LIBS"
    CPPFLAGS="$CPPFLAGS $PCSC_CFLAGS"
    LIBS="$LDFLAGS $PCSC_LIBS"

    AC_CHECK_HEADERS(winscard.h, [], [ AC_MSG_ERROR([winscard.h not found, install PC/SC Lite or similar or use ./configure PCSC_CFLAGS=...]) ])
    AC_CHECK_HEADERS(reader.h, [AC_DEFINE(HAVE_READER_H, 1, [use reader.h from PC/SC Lite])], [])
    AC_CHECK_HEADERS(pcsclite.h, [AC_DEFINE(HAVE_PCSCLITE_H, 1, [use pcsclite.h from PC/SC Lite])], [])
    AC_CHECK_DECL(FEATURE_EXECUTE_PACE, [],
                    [AC_MSG_ERROR([libpcsclite > 1.4.102 patched with pcsclite_trunk.patch not found, use ./configure PCSC_LIBS=...]) ],
                    [#include <reader.h>])
    AC_MSG_CHECKING([for SCardEstablishContext])
    AC_TRY_LINK_FUNC(SCardEstablishContext, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libpcsclite > 1.4.102 not found, use ./configure PCSC_LIBS=...]) ])

    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"

    enable_cats=yes
fi
AM_CONDITIONAL(WITH_CATS, test "x${enable_cats}" != "xno")


# Documentation (default: no)
AC_ARG_ENABLE([doc],AS_HELP_STRING([--enable-doc],[Enable documentation generation.]),[enable_doc=$enableval],[enable_doc="no"])
if test x"$enable_doc" = "xyes"
then
    AC_PATH_PROG([DOXYGEN], [doxygen])
    if test x$DOXYGEN = x
    then
        AC_MSG_ERROR([doxygen is mandatory.])
    fi
fi
AM_CONDITIONAL(DOC_ENABLED, [test x"$enable_doc" = xyes])


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
OPENSC_CFLAGS:        ${OPENSC_CFLAGS}
OPENSC_LIBS:          ${OPENSC_LIBS}
OPENSSL_CFLAGS:       ${OPENSSL_CFLAGS}
OPENSSL_LIBS:         ${OPENSSL_LIBS}
LIBPACE_CFLAGS:       ${LIBPACE_CFLAGS}
LIBPACE_LIBS:         ${LIBPACE_LIBS}
PCSC_CFLAGS:          ${PCSC_CFLAGS}
PCSC_LIBS:            ${PCSC_LIBS}

CCID emulator:        ${enable_ccid}
PACE support:         ${enable_pace}
CAT-S/CAT-C Tool:     ${enable_cats}
Documentation:        ${enable_doc}

EOF

AC_CONFIG_FILES([
                 Makefile
                 m4/Makefile
                 patches/Makefile
                 src/Makefile
                 ])
AC_OUTPUT
