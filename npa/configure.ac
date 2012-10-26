#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([nPA Smart Card Library], [0.5], [http://sourceforge.net/projects/vsmartcard/support], [npa], [http://vsmartcard.sourceforge.net/npa/README.html])
AC_CONFIG_SRCDIR([src/npa.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

dnl Add libtool support.
ifdef(
	[LT_INIT],
	[
		LT_INIT([win32-dll])
	],
	[
		AC_LIBTOOL_WIN32_DLL
		AC_PROG_LIBTOOL
	]
)

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_SED
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG
AC_ARG_VAR([HELP2MAN],
           [absolute path to help2man used for man page generation of npa-tool])
AC_PATH_PROG(HELP2MAN, help2man, not found)
if test ! -r src/npa-tool.1 -a "${HELP2MAN}" = "not found"
then
    AC_MSG_ERROR([Need help2man to generate man page for npa-tool])
fi
AC_ARG_VAR([GENGETOPT],
           [absolute path to gengetopt used for command line parsing of npa-tool])
AC_PATH_PROG(GENGETOPT, gengetopt, not found)
if test ! -r src/cmdline.h -a "${GENGETOPT}" = "not found"
then
    AC_MSG_ERROR([Need gengetopt for parsing command line of npa-tool])
fi

# If you need to see the details, just run make V=1.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])


# Checks for libraries.
PKG_CHECK_EXISTS([libopensc],
                 [PKG_CHECK_MODULES([OPENSC], [libopensc >= 0.12.1 ])],
                 [AC_MSG_WARN([libopensc >= 0.12.1 not found by pkg-config])])
test -z "$OPENSC_LIBS" && OPENSC_LIBS="-lopensc"

saved_LIBS="$LIBS"
LIBS="$LDFLAGS $OPENSC_LIBS"

AC_MSG_CHECKING([for sc_perform_pace])
AC_TRY_LINK_FUNC(sc_perform_pace, [ AC_MSG_RESULT([yes]) ],
        [ AC_MSG_ERROR([libopensc with PACE (git clone git://github.com/frankmorgner/OpenSC.git) not found, use ./configure OPENSC_LIBS=...]) ])

LIBS="$saved_LIBS"


PKG_CHECK_EXISTS([libcrypto],
                 [PKG_CHECK_MODULES([OPENSSL], [libcrypto])],
                 [AC_MSG_WARN([libcrypto not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENSSL_CFLAGS"
LIBS="$LDFLAGS $OPENSSL_LIBS"

AC_CHECK_HEADERS(openssl/evp.h, [], [ AC_MSG_ERROR([openssl/evp.h not found, install OpenSSL or use ./configure OPENSSL_CFLAGS=...]) ])
AC_MSG_CHECKING([for EVP_read_pw_string_min])
AC_TRY_LINK_FUNC(EVP_read_pw_string_min, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([OpenSSL not found, use ./configure OPENSSL_LIBS=...]) ])

CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"


PKG_CHECK_EXISTS([libeac],
                 [PKG_CHECK_MODULES([OPENPACE], [libeac])],
                 [AC_MSG_WARN([libeac not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENPACE_CFLAGS"
LIBS="$LDFLAGS $OPENPACE_LIBS"

AC_CHECK_HEADERS(eac/eac.h, [], [ AC_MSG_ERROR([eac/eac.h not found, install OpenPACE or use ./configure OPENSSL_CFLAGS=...]) ])
AC_MSG_CHECKING([for EAC_CTX_init_pace])
AC_TRY_LINK_FUNC(EAC_CTX_init_pace, [ AC_MSG_RESULT([yes]) ], [ AC_MSG_ERROR([OpenPACE not found, use ./configure OPENPACE_LIBS=...]) ])

CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"



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
AC_CHECK_FUNCS([getline memmove memset strerror gettimeofday strtol])

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
OPENPACE_CFLAGS:      ${OPENPACE_CFLAGS}
OPENPACE_LIBS:        ${OPENPACE_LIBS}


HELP2MAN:             ${HELP2MAN}
GENGETOPT:            ${GENGETOPT}

EOF

AC_CONFIG_FILES([
                 Makefile
                 m4/Makefile
                 src/Makefile
                 win32/Makefile
                 doc/Makefile
                 ])
AC_OUTPUT
