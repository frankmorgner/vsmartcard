#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
AC_INIT([virtualsmartcard], [0.2], [http://sourceforge.net/projects/vsmartcard/support])
AC_CONFIG_SRCDIR([src/vpcd/ifd-vpcd.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
AM_PATH_PYTHON
PKG_PROG_PKG_CONFIG


# Checks for libraries.
PKG_CHECK_EXISTS([libpcsclite],
                 [PKG_CHECK_MODULES([PCSC], [libpcsclite])],
                 [AC_MSG_WARN([libpcsclite not found by pkg-config])])

saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PCSC_CFLAGS"
AC_CHECK_HEADERS(ifdhandler.h,,
        [ AC_MSG_ERROR([ifdhandler.h not found, install libpcsclite or use ./configure PCSC_CFLAGS=...]) ])
CPPFLAGS="$saved_CPPFLAGS"


# --enable-serialdropdir=DIR
AC_ARG_ENABLE(serialdropdir,
	AC_HELP_STRING([--enable-serialdropdir=DIR],[directory to install the
	serial wrapper driver (default to pcscd config or $(prefix)/pcsc/drivers/serial)]),
	[serialdropdir="${enableval}"], [serialdropdir=false])
if test "${serialdropdir}" = false ; then
	if test "${prefix}" = NONE ; then
		serialdropdir=`$PKG_CONFIG libpcsclite --variable=usbdropdir`/serial
		if test "${serialdropdir}" = "/serial" ; then
			AC_MSG_ERROR([use --enable-serialdropdir=DIR])
		fi
	else
		serialdropdir=${prefix}`$PKG_CONFIG libpcsclite --variable=usbdropdir`/serial
		if test "${serialdropdir}" = "${prefix}/serial" ; then
			AC_MSG_ERROR([use --enable-serialdropdir=DIR])
		fi
	fi
fi


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset select socket])

# Select OS specific versions of source files.
AC_SUBST(BUNDLE_HOST)
AC_SUBST(DYN_LIB_EXT)
AC_SUBST(serialdropdir)
case "$host" in
*-*-darwin*)
	BUNDLE_HOST="MacOS"
	DYN_LIB_EXT="dylib"
	;;
*-*-freebsd*)
	BUNDLE_HOST="FreeBSD"
	DYN_LIB_EXT="so"
	;;
*-*-openbsd*)
	BUNDLE_HOST="OpenBSD"
	DYN_LIB_EXT="so.0.0"
	;;
*-*-solaris*)
	BUNDLE_HOST="Solaris"
	DYN_LIB_EXT="so"
	;;
*-*-dragonfly*)
	BUNDLE_HOST="DragonFly"
	DYN_LIB_EXT="so"
	;;
*)
	BUNDLE_HOST="Linux"
	DYN_LIB_EXT="so"
esac

cat << EOF

${PACKAGE} has been configured with following options:

Version:              ${PACKAGE_VERSION}
User binaries:        $(eval eval eval echo "${bindir}")
Python site-packages: $(eval eval eval echo "${pythondir}")
Driver directory:     ${serialdropdir}


Host:                 ${host}
Compiler:             ${CC}
Preprocessor flags:   ${CPPFLAGS}
Compiler flags:       ${CFLAGS}
Preprocessor flags:   ${CPPFLAGS}
Linker flags:         ${LDFLAGS}
Libraries:            ${LIBS}
PCSC_CFLAGS:          ${PCSC_CFLAGS}
PCSC_LIBS:            ${PCSC_LIBS}
BUNDLE_HOST:          ${BUNDLE_HOST}
DYN_LIB_EXT:          ${DYN_LIB_EXT}

EOF

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/vpcd/Makefile
                 src/vpicc/Makefile
                 ])
AC_OUTPUT
