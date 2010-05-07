#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([ccid], [0.2], [morgner@informatik.hu-berlin.de])
AC_CONFIG_SRCDIR([src/ccid.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PKG_CHECK_EXISTS([libopensc],
                 [PKG_CHECK_MODULES([OPENSC], [libopensc])],
                 [AC_MSG_WARN([libopensc not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENSC_CFLAGS"
LIBS="$LDFLAGS $OPENSC_LIBS"
AC_CHECK_HEADERS(opensc/opensc.h,,
        [ AC_MSG_ERROR([opensc/opensc.h not found, install libopensc or use ./configure OPENSC_CFLAGS=...]) ])
AC_MSG_CHECKING([for sc_context_create])
AC_TRY_LINK_FUNC(sc_context_create, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libopensc not found, use ./configure OPENSC_LIBS=...]) ])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"

# --disable-ccid
AC_ARG_ENABLE(ccid,
              AS_HELP_STRING([--disable-ccid], [Disable USB CCID emulator]),
              [enable_ccid="${enableval}"], [enable_ccid=yes])
if test "x$enable_ccid" != xno ; then
    AC_CHECK_HEADERS(linux/usb/gadgetfs.h, [],
                     [ AC_MSG_ERROR([linux/usb/gadgetfs.h not found]) ])

    ACX_PTHREAD

    saved_CPPFLAGS="$CPPFLAGS"
    saved_LIBS="$LIBS"
    CPPFLAGS="$CPPFLAGS $PTHREAD_CFLAGS"
    LIBS="$LDFLAGS $PTHREAD_LIBS"

    AC_CHECK_HEADERS(pthread.h, [],
                     [ AC_MSG_ERROR([pthread.h not found, use ./configure PTHREAD_CFLAGS=...]) ])
    AC_MSG_CHECKING([for parse_ef_card_access])
    AC_TRY_LINK_FUNC(pthread_create, [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_ERROR([pthread not found, use ./configure PTHREAD_LIBS=...]) ])

    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"

    enable_ccid=yes
fi
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)
AM_CONDITIONAL(WITH_CCID, test "${enable_ccid}" != "no")

# --enable-pace
AC_ARG_ENABLE(pace,
              AS_HELP_STRING([--enable-pace], [Enable Password Authenticated Connection Establishment (PACE)]),
              [enable_pace="${enableval}"], [enable_pace=no])
if test "x$enable_pace" != xno ; then
    PKG_CHECK_EXISTS([libssl],
                     [PKG_CHECK_MODULES([OPENSSL], [libssl])],
                     [AC_MSG_WARN([libssl not found by pkg-config])])

    saved_CPPFLAGS="$CPPFLAGS"
    saved_LIBS="$LIBS"
    CPPFLAGS="$CPPFLAGS $OPENSSL_CFLAGS"
    LIBS="$LDFLAGS $OPENSSL_LIBS"

    AC_CHECK_HEADERS(openssl/pace.h, [], [ AC_MSG_ERROR([openssl/pace.h not found, install OpenPACE or use ./configure OPENSSL_CFLAGS=...]) ])
    AC_MSG_CHECKING([for parse_ef_card_access])
    AC_TRY_LINK_FUNC(parse_ef_card_access, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([OpenPACE not found, use ./configure OPENSSL_LIBS=...]) ])

    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"

    enable_pace=yes
fi
AC_SUBST(OPENSSL_CFLAGS)
AC_SUBST(OPENSSL_LIBS)
AM_CONDITIONAL(WITH_PACE, test "${enable_pace}" != "no")


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
AC_CHECK_FUNCS([memmove memset strerror])

cat << EOF

ccid has been configured with following options:

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
PTHREAD_CFLAGS:       ${PTHREAD_CFLAGS}
PTHREAD_LIBS:         ${PTHREAD_LIBS}
OPENSC_CFLAGS:        ${OPENSC_CFLAGS}
OPENSC_LIBS:          ${OPENSC_LIBS}
OPENSSL_CFLAGS:       ${OPENSSL_CFLAGS}
OPENSSL_LIBS:         ${OPENSSL_LIBS}

ccid support:         ${enable_ccid}
pace support:         ${enable_pace}

EOF

AC_CONFIG_FILES([Makefile
                 m4/Makefile
                 src/Makefile])
AC_OUTPUT
