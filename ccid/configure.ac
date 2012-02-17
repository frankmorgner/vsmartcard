#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([CCID Emulator], [0.6], [http://sourceforge.net/projects/vsmartcard/support], [ccid-emulator], [http://sourceforge.net/projects/vsmartcard/])
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

# If you need to see the details, just run make V=1.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for libraries.

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
AM_CONDITIONAL(WITH_CCID, test "${enable_ccid}" != "no")


PKG_CHECK_EXISTS([libssl],
                 [PKG_CHECK_MODULES([OPENSSL], [libssl >= 1.0.0])],
                 [AC_MSG_WARN([libssl not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENSSL_CFLAGS"
LIBS="$LDFLAGS $OPENSSL_LIBS"
AC_CHECK_HEADERS(openssl/evp.h, [], [ AC_MSG_ERROR([openssl/evp.h not found, install OpenSSL >= 1.0.0 or use ./configure OPENSSL_CFLAGS=...]) ])
AC_MSG_CHECKING([for EVP_read_pw_string_min])
AC_TRY_LINK_FUNC(EVP_read_pw_string_min, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([OpenSSL >= 1.0.0 not found, use ./configure OPENSSL_LIBS=...]) ])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"

# --enable-pace
AC_ARG_ENABLE(pace,
              AS_HELP_STRING([--enable-pace], [Enable Password Authenticated Connection Establishment (PACE)]),
              [enable_pace="${enableval}"], [enable_pace=no])
saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
if test "x$enable_pace" != xno ; then
    PKG_CHECK_EXISTS([libnpa],
                     [PKG_CHECK_MODULES([LIBNPA], [libnpa])],
                     [AC_MSG_WARN([libnpa not found by pkg-config])])
    CPPFLAGS="$CPPFLAGS $LIBNPA_CFLAGS -I$(pwd)/src/opensc/src $OPENSSL_CFLAGS"
    LIBS="$LDFLAGS $LIBNPA_LIBS $OPENSSL_LIBS"
    AC_CHECK_HEADERS(npa/npa.h, [], [ AC_MSG_ERROR([npa/npa.h not found, install libnpa or use ./configure LIBNPA_CFLAGS=...]) ])
    AC_MSG_CHECKING([for EstablishPACEChannel])
    AC_TRY_LINK_FUNC(EstablishPACEChannel, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([libnpa not found, use ./configure LIBNPA_LIBS=...]) ])

    enable_pace=yes
else
    PKG_CHECK_EXISTS([libopensc],
                     [PKG_CHECK_MODULES([OPENSC], [libopensc >= 0.12.1])],
                     [AC_MSG_WARN([libopensc >= 0.12.1 not found by pkg-config])])
    test -z "$OPENSC_LIBS" && OPENSC_LIBS="-lopensc"
    LIBS="$LDFLAGS $OPENSC_LIBS $OPENSSL_LIBS"
    AC_MSG_CHECKING([for sc_bytes2apdu])
    AC_TRY_LINK_FUNC(sc_bytes2apdu, [ AC_MSG_RESULT([yes]) ],
    [ AC_MSG_ERROR([libopensc >= 0.12.1 not found, use ./configure OPENSC_LIBS=...]) ])
fi
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"

AM_CONDITIONAL(WITH_PACE, test "${enable_pace}" != "no")
AM_COND_IF(WITH_PACE, [AC_DEFINE(WITH_PACE, 1, [enable PACE support])])


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
OPENSC_LIBS:          ${OPENSC_LIBS}
OPENSSL_CFLAGS:       ${OPENSSL_CFLAGS}
OPENSSL_LIBS:         ${OPENSSL_LIBS}
LIBNPA_CFLAGS:        ${LIBNPA_CFLAGS}
LIBNPA_LIBS:          ${LIBNPA_LIBS}

CCID emulator:        ${enable_ccid}
PACE support:         ${enable_pace}

EOF

AC_CONFIG_FILES([
                 Makefile
                 m4/Makefile
                 patches/Makefile
                 src/Makefile
                 doc/Makefile
                 ])
AC_OUTPUT
