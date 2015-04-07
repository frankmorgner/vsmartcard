#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([nPA Smart Card Library], [0.7], [https://github.com/frankmorgner/vsmartcard/issues], [npa], [http://frankmorgner.github.io/vsmartcard/npa/README.html])
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
test -z "$OPENSC_LIBS" && OPENSC_LIBS="-lopensc"
test -z "$OPENSC_CFLAGS" && OPENSC_CFLAGS="-I${PWD}/${srcdir}/src/opensc/src -DENABLE_SM"

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$OPENSC_CFLAGS $CPPFLAGS"
LIBS="$LDFLAGS $OPENSC_LIBS"

AC_CHECK_HEADERS([libopensc/opensc.h libopensc/sm.h libopensc/pace.h],
                 [], [AC_MSG_ERROR([OpenSC headers not found])])
AC_MSG_CHECKING([for sc_sm_stop])
AC_TRY_LINK_FUNC(sc_sm_stop,
                 [ AC_MSG_RESULT([yes]) ],
                 [ AC_MSG_ERROR([Cannot link against libopensc with SM]) ])
AC_CHECK_FUNCS([sc_apdu_get_octets _sc_match_atr list_iterator_next])

CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"
AC_SUBST([OPENSC_LIBS])
AC_SUBST([OPENSC_CFLAGS])


PKG_CHECK_EXISTS([libcrypto],
                 [PKG_CHECK_MODULES([OPENSSL], [libcrypto])],
                 [AC_MSG_WARN([libcrypto not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENSSL_CFLAGS"
LIBS="$LDFLAGS $OPENSSL_LIBS"

AC_CHECK_HEADERS(openssl/evp.h, [],
                 [ AC_MSG_ERROR([OpenSSL headers not found]) ])
AC_MSG_CHECKING([for EVP_read_pw_string_min])
AC_TRY_LINK_FUNC(EVP_read_pw_string_min,
                 [ AC_MSG_RESULT([yes]) ],
                 [ AC_MSG_ERROR([Cannot link against libcrypto]) ])

CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"


PKG_CHECK_EXISTS([libeac], [PKG_CHECK_MODULES([OPENPACE], [libeac >= 0.9])],
                 [AC_MSG_WARN([libeac not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $OPENPACE_CFLAGS"
LIBS="$LDFLAGS $OPENPACE_LIBS"

AC_CHECK_HEADERS(eac/eac.h, [],
                 [ AC_MSG_ERROR([OpenPACE headers not found]) ])
AC_MSG_CHECKING([for EAC_CTX_init_pace])
AC_TRY_LINK_FUNC(EAC_CTX_init_pace, [ AC_MSG_RESULT([yes]) ],
                 [ AC_MSG_ERROR([Cannot link against libeac]) ])

CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"


AC_ARG_ENABLE(cvcdir,
              AC_HELP_STRING([--enable-cvcdir=DIR],
                             [directory containing CV certificates (default is determined by libeac)]),
                             [cvcdir="${enableval}"],
                             [cvcdir=false])
if test "${cvcdir}" = false ; then
    cvcdir="`$PKG_CONFIG libeac --variable=cvcdir`"
fi
if test "${cvcdir}" = "" ; then
    AC_MSG_ERROR([use --enable-cvcdir=DIR])
fi
CVCDIR="${cvcdir}"
AC_SUBST(CVCDIR)

AC_ARG_ENABLE(x509dir,
              AC_HELP_STRING([--enable-x509dir=DIR],
                             [directory containing X.509 certificates (default is determined by libeac)]),
                             [x509dir="${enableval}"],
                             [x509dir=false])
if test "${x509dir}" = false ; then
    x509dir="`$PKG_CONFIG libeac --variable=x509dir`"
fi
if test -z "${x509dir}"
then
    x509dir="`$PKG_CONFIG libeac --variable=x509dir`"
fi
if test -z "${x509dir}"
then
    AC_MSG_ERROR([use --enable-x509dir=DIR])
fi
X509DIR="${x509dir}"
AC_SUBST(X509DIR)


PACKAGE_SUMMARY="Access the German electronic identity card (neuer Personalausweis/nPA)"
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
AC_CHECK_FUNCS([getline memmove memset strerror gettimeofday strtol])

AC_SUBST(DYN_LIB_EXT)
case "$host" in
*-*-darwin*)
	DYN_LIB_EXT="dylib"
	;;
*-*-freebsd*)
	DYN_LIB_EXT="so"
	;;
*-*-openbsd*)
	DYN_LIB_EXT="so.0.0"
	;;
*-*-solaris*)
	DYN_LIB_EXT="so"
	;;
*-*-dragonfly*)
	DYN_LIB_EXT="so"
	;;
*-*-cygwin*)
	DYN_LIB_EXT="dll"
	;;
*)
	DYN_LIB_EXT="so"
esac

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
OPENSC_CFLAGS:        ${OPENSC_CFLAGS}
OPENSSL_CFLAGS:       ${OPENSSL_CFLAGS}
OPENSSL_LIBS:         ${OPENSSL_LIBS}
OPENPACE_CFLAGS:      ${OPENPACE_CFLAGS}
OPENPACE_LIBS:        ${OPENPACE_LIBS}


HELP2MAN:             ${HELP2MAN}
GENGETOPT:            ${GENGETOPT}

EOF

AC_CONFIG_FILES([
                 Makefile
                 certs/Makefile
                 doc/Makefile
                 m4/Makefile
                 src/Makefile
                 cross/Makefile
                 ])
AC_OUTPUT
