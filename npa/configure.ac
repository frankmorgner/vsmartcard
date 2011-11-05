#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([nPA smart card library], [0.4], [http://sourceforge.net/projects/vsmartcard/support], [npa], [http://sourceforge.net/projects/vsmartcard/])
AC_CONFIG_SRCDIR([src/npa.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
LT_INIT
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

# If you need to see the details, just run make V=1.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for libraries.
PKG_CHECK_EXISTS([libopensc],
                 [PKG_CHECK_MODULES([OPENSC], [libopensc >= 0.12.1 ])],
                 [AC_MSG_WARN([libopensc >= 0.12.1 not found by pkg-config])])
test -z "$OPENSC_LIBS" && OPENSC_LIBS="-lopensc"

PKG_CHECK_EXISTS([libssl],
                 [PKG_CHECK_MODULES([OPENSSL], [libssl])],
                 [AC_MSG_WARN([libssl not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENSSL_CFLAGS"
LIBS="$LDFLAGS $OPENSC_LIBS $OPENSSL_LIBS"
AC_MSG_CHECKING([for sc_bytes2apdu])
AC_TRY_LINK_FUNC(sc_bytes2apdu, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libopensc >= 0.12.1 not found, use ./configure OPENSC_LIBS=...]) ])

AC_CHECK_HEADERS(openssl/evp.h, [], [ AC_MSG_ERROR([openssl/evp.h not found, install OpenSSL or use ./configure OPENSSL_CFLAGS=...]) ])
AC_MSG_CHECKING([for EVP_read_pw_string_min])
AC_TRY_LINK_FUNC(EVP_read_pw_string_min, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([OpenSSL not found, use ./configure OPENSSL_LIBS=...]) ])

AC_CHECK_HEADERS(openssl/eac.h, [], [ AC_MSG_ERROR([openssl/eac.h not found, install OpenPACE or use ./configure OPENSSL_CFLAGS=...]) ])
AC_MSG_CHECKING([for EAC_CTX_init_pace])
AC_TRY_LINK_FUNC(EAC_CTX_init_pace, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([OpenPACE not found, use ./configure OPENSSL_LIBS=...]) ])

CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"

AC_DEFINE(BUERGERCLIENT_WORKAROUND, 1, [Always get EF.CardAccess, when connecting to a smart card. This is a workaround for the recent Buergerclient])


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
AC_CHECK_FUNCS([getline memmove memset strerror])

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
OPENSC_LIBS:          ${OPENSC_LIBS}
OPENSSL_CFLAGS:       ${OPENSSL_CFLAGS}
OPENSSL_LIBS:         ${OPENSSL_LIBS}

EOF

AC_CONFIG_FILES([
                 Makefile
                 m4/Makefile
                 src/Makefile
                 doc/Makefile
                 ])
AC_OUTPUT
