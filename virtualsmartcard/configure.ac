#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.72])
AC_INIT([Virtual Smart Card],[0.10],[https://github.com/frankmorgner/vsmartcard/issues],[virtualsmartcard],[http://frankmorgner.github.io/vsmartcard/virtualsmartcard/README.html])
AC_CONFIG_SRCDIR([src/vpcd/vpcd.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE(foreign)

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

dnl Check for windows build
case "${host}" in
    *-mingw*|*-winnt*)
        WIN32="yes"
        ;;
    *-cygwin*)
        WIN32="yes"
        ;;
    *-*-darwin*)
        OSX="yes"
        OSX_VERSION="$(sw_vers -productVersion | awk -F '.' '{print $1 * 1000 + $2}')"
        OSX_GE_CAPITAN=$(test "${OSX_VERSION}" -gt 10011 && echo "yes")
        OSX_SERIAL_LOCAL_PREFIX=$(test "${OSX_GE_CAPITAN}" = "yes" && echo "/local")
        ;;
esac
AM_CONDITIONAL([WIN32], [test "${WIN32}" = "yes"])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_SED
AM_PATH_PYTHON([3.2])
AC_PROG_CXX
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG
AX_PTHREAD
AC_ARG_VAR([HELP2MAN],
           [absolute path to help2man used for man page generation of vicc])
AC_PATH_PROG(HELP2MAN, help2man, not found)
if test ! -r src/vicc.1
then
    if test "${HELP2MAN}" = "not found"
    then
        AC_MSG_ERROR([Need help2man to generate man page for vicc])
    fi
else
    # make sure these files are the most recent
    touch src/vicc.1
fi

# If you need to see the details, just run make V=1.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])


# Checks for libraries.

# --enable-infoplist
AC_ARG_ENABLE(infoplist,
	AS_HELP_STRING([--enable-infoplist],[Builds Info.plist for OSX @<:@default=no@:>@]),
	[infoplist="yes"], [infoplist=no])

# --enable-libpcsclite
AC_ARG_ENABLE(libpcsclite,
	AS_HELP_STRING([--enable-libpcsclite],[Build a lightweight version of
					libpcsclite, conflicts with PCSC-Lite @<:@default=no@:>@]),
	[libpcsclite="${enableval}"], [libpcsclite=no])
if test "${libpcsclite}" = no ; then
	PKG_CHECK_EXISTS([libpcsclite],
					 [PKG_CHECK_MODULES([PCSC], [libpcsclite])],
					 [AC_MSG_WARN([libpcsclite not found by pkg-config])
					 case "$host" in
						 *-*-darwin*)
							SDKS_PATH="$(xcode-select -p)/Platforms/MacOSX.platform/Developer/SDKs"
							SDK_PATH="${SDK_PATH:-$SDKS_PATH/$(ls -1 ${SDKS_PATH} | grep -i MacOSX.sdk | sort -n -t. -k2 -r | head -1)}"
							# and set the PC/SC include path
							PCSC_CFLAGS="-I$SDK_PATH/System/Library/Frameworks/PCSC.framework/Versions/Current/Headers -DRESPONSECODE_DEFINED_IN_WINTYPES_H -I`pwd`/${top_srcdir}/MacOSX"
							PCSC_LIBS="--framework PCSC"
					 esac
					 ])

	saved_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS $PCSC_CFLAGS"
	AC_CHECK_HEADERS(ifdhandler.h,,
			[ AC_MSG_ERROR([ifdhandler.h not found, install libpcsclite or use ./configure PCSC_CFLAGS=...])], [#include <wintypes.h>])
	AC_CHECK_HEADERS([debuglog.h])
	CPPFLAGS="$saved_CPPFLAGS"


	# --enable-serialdropdir=DIR
	AC_ARG_ENABLE(serialdropdir,
		AS_HELP_STRING([--enable-serialdropdir=DIR],[directory to install the
		serial wrapper driver (default to pcscd config or $(prefix)/pcsc/drivers/serial)]),
		[serialdropdir="${enableval}"], [serialdropdir=no])
	if test "${serialdropdir}" = no ; then
		if test "${prefix}" = NONE ; then
			serialdropdir="`$PKG_CONFIG libpcsclite --variable=usbdropdir`/serial"
			if test "${serialdropdir}" = "/serial" ; then
				case "$host" in
					*-*-darwin*)
						MAC_VERSION=$(sw_vers -productVersion | awk -F '.' '{print $1 * 1000 + $2}')
						if test "$MAC_VERSION" -ge "10011"; then
							# System Integrity Protection means we can't write
							# to /usr/libexec; even as root!
							serialdropdir=/usr/local/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Contents/MacOS
						else
							serialdropdir=/usr/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Contents/MacOS
						fi
						infoplist=yes
						;;
					*) AC_MSG_ERROR([use --enable-serialdropdir=DIR])
				esac
			fi
		else
			serialdropdir="${prefix}`$PKG_CONFIG libpcsclite --variable=usbdropdir`/serial"
			if test "${serialdropdir}" = "${prefix}/serial" ; then
				case "$host" in
					*-*-darwin*)
						MAC_VERSION=$(sw_vers -productVersion | awk -F '.' '{print $1 * 1000 + $2}')
						if test "$MAC_VERSION" -ge "10011"; then
							# System Integrity Protection means we can't write
							# to /usr/libexec; even as root!
							serialdropdir=${prefix}/usr/local/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Contents/MacOS
						else
							serialdropdir=${prefix}/usr/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Contents/MacOS
						fi
						infoplist=yes
						;;
					*) AC_MSG_ERROR([use --enable-serialdropdir=DIR])
				esac
			fi
		fi
	fi


	# --enable-serialconfdir=DIR
	AC_ARG_ENABLE(serialconfdir,
		AS_HELP_STRING([--enable-serialconfdir=DIR],[directory to install the
		serial wrapper driver (default to pcscd config or /etc/reader.conf.d)]),
		[serialconfdir="${enableval}"], [serialconfdir=no])
	if test "${serialconfdir}" = no ; then
		if test "${prefix}" = NONE ; then
			serialconfdir="`$PKG_CONFIG libpcsclite --variable=serialconfdir`"
			if test "${serialconfdir}" = "" ; then
				case "$host" in
					*-*-darwin*)
					MAC_VERSION=$(sw_vers -productVersion | awk -F '.' '{print $1 * 1000 + $2}')
					if test "$MAC_VERSION" -ge "10011"; then
						# System Integrity Protection means we can't write
						# to /usr/libexec; even as root!
						serialconfdir=/usr/local/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Contents
					else
						serialconfdir=/usr/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Contents
					fi
					infoplist=yes
					;;
					*) serialconfdir="/etc/reader.conf.d"
				esac
			fi
		else
			serialconfdir="${prefix}`$PKG_CONFIG libpcsclite --variable=serialconfdir`"
			if test "${serialconfdir}" = "${prefix}" ; then
				case "$host" in
					*-*-darwin*)
					MAC_VERSION=$(sw_vers -productVersion | awk -F '.' '{print $1 * 1000 + $2}')
					if test "$MAC_VERSION" -ge "10011"; then
						# System Integrity Protection means we can't write
						# to /usr/libexec; even as root!
						serialconfdir=${prefix}/usr/local/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Contents
					else
						serialconfdir=${prefix}/usr/libexec/SmartCardServices/drivers/ifd-vpcd.bundle/Contents
					fi
					infoplist=yes
					;;
					*) serialconfdir="${prefix}/etc/reader.conf.d"
				esac
			fi
		fi
	fi
else
	builddir="`pwd`"
	abs_srcdir="`cd ${srcdir}; pwd`"
	PCSC_CFLAGS="-DNO_LOG -I${abs_srcdir}/src/pcsclite-vpcd/PCSC -I${builddir}/src/pcsclite-vpcd/PCSC"
	PCSC_LIBS="${builddir}/src/pcsclite-vpcd/libpcsclite.la"
	AC_SUBST(PCSC_CFLAGS)
	AC_SUBST(PCSC_LIBS)
fi


# --enable-vpcdhost
AC_ARG_ENABLE(vpcdhost,
	AS_HELP_STRING([--enable-vpcdhost=ADDRESS],[Default address to connect to when
					communicating with vicc. Use "/dev/null" if vpcd shall open
					the socket and wait for an incoming connection.
					@<:@default=/dev/null@:>@]),
	[vpcdhost="${enableval}"], [vpcdhost=/dev/null])
AC_SUBST(vpcdhost)
if test "${vpcdhost}" = "/dev/null"; then
	AC_DEFINE_UNQUOTED(VPCDHOST, NULL, [address of vicc])
else
	AC_DEFINE_UNQUOTED(VPCDHOST, "${vpcdhost}", [address of vicc])
fi


# --enable-vpcdslots
AC_ARG_ENABLE(vpcdslots,
	AS_HELP_STRING([--enable-vpcdslots=COUNT],[Default number of slots to open.
					vpcd will open one socket for each slot incrementing the
					port number each time. @<:@default=2@:>@]),
	[vpcdslots="${enableval}"], [vpcdslots=2])
AC_SUBST(vpcdslots)
AC_DEFINE_UNQUOTED(VPCDSLOTS, ${vpcdslots}, [number of vpcd slots])


HAVE_QRENCODE=yes
PKG_CHECK_EXISTS([libqrencode],
				 [PKG_CHECK_MODULES([QRENCODE], [libqrencode])],
				 [AC_MSG_WARN([libqrencode not found by pkg-config])])
CPPFLAGS="$CPPFLAGS $QRENCODE_CFLAGS -I${srcdir}/src/opensc/src"
LIBS="$LDFLAGS $QRENCODE_LIBS"
AC_CHECK_HEADERS(qrencode.h, [], [ HAVE_QRENCODE=no ])
AC_MSG_CHECKING([for QRcode_encodeString])
AC_TRY_LINK_FUNC(QRcode_encodeString, [ AC_MSG_RESULT([yes]) ], [ HAVE_QRENCODE=no ])
if test "${HAVE_QRENCODE}" = "yes"; then
	AC_DEFINE(HAVE_QRENCODE, 1, [enable QR support])
fi
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"



PACKAGE_SUMMARY="Smart card emulator written in Python"
AC_SUBST(PACKAGE_SUMMARY)


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h stdint.h stdlib.h string.h sys/socket.h sys/time.h unistd.h syslog.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_CHECK_DECLS([SO_NOSIGPIPE], [], [], [#include <sys/socket.h>])
AC_CHECK_DECLS([MSG_NOSIGNAL], [], [], [#include <sys/socket.h>])
if test "${WIN32}" = "yes"
then
	saved_CPPFLAGS="$CPPFLAGS"
	CPPFLAGS="$CPPFLAGS -D_WIN32_WINNT=0x0600"
	AC_CHECK_DECLS([inet_ntop], [], [], [#include <ws2tcpip.h>])
	CPPFLAGS="$saved_CPPFLAGS"
else
	AC_CHECK_DECLS([inet_ntop], [], [], [#include <arpa/inet.h>])
fi
 
# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset select socket])
AC_CHECK_FUNCS([strlcpy strlcat], [], [], [#include <string.h>])

# Select OS specific versions of source files.
AM_CONDITIONAL([BUILD_LIBPCSCLITE], [test "${libpcsclite}" = "yes"])
AM_CONDITIONAL([BUILD_INFOPLIST], [test "${infoplist}" = "yes"])
AC_SUBST(BUNDLE_HOST)
AC_SUBST(DYN_LIB_EXT)
AC_SUBST(LIB_PREFIX)
AC_SUBST(serialdropdir)
AC_SUBST(serialconfdir)
AC_SUBST(OSX_SERIAL_LOCAL_PREFIX)
case "$host" in
*-*-darwin*)
	BUNDLE_HOST="MacOS"
	DYN_LIB_EXT="dylib"
	LIB_PREFIX="lib"
	;;
*-*-freebsd*)
	BUNDLE_HOST="FreeBSD"
	DYN_LIB_EXT="so"
	LIB_PREFIX="lib"
	;;
*-*-openbsd*)
	BUNDLE_HOST="OpenBSD"
	DYN_LIB_EXT="so.0.0"
	LIB_PREFIX="lib"
	;;
*-*-solaris*)
	BUNDLE_HOST="Solaris"
	DYN_LIB_EXT="so"
	LIB_PREFIX="lib"
	;;
*-*-dragonfly*)
	BUNDLE_HOST="DragonFly"
	DYN_LIB_EXT="so"
	LIB_PREFIX="lib"
	;;
*-*-cygwin*)
	BUNDLE_HOST="Cygwin"
	DYN_LIB_EXT="dll"
	LIB_PREFIX="cyg"
	;;
*)
	BUNDLE_HOST="Linux"
	DYN_LIB_EXT="so"
	LIB_PREFIX="lib"
esac


AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 doc/api/Makefile
                 npa-example-data/Makefile
                 npa-example-data/dh/Makefile
                 npa-example-data/ecdh/Makefile
                 src/Makefile
                 src/pcsclite-vpcd/Makefile
                 src/ifd-vpcd/Makefile
                 src/vpcd/Makefile
                 src/vpicc/Makefile
                 src/vpcd-config/Makefile
                 MacOSX/Makefile
                 ])
AC_OUTPUT

cat << EOF

${PACKAGE} has been configured with following options:

Version:              ${PACKAGE_VERSION}
User binaries:        $(eval eval eval echo "${bindir}")
PC/SC configuration:  ${serialconfdir}
Driver directory:     ${serialdropdir}
Python site-packages: $(eval eval eval echo "${pythondir}")

Build libpcsclite:    ${libpcsclite}
Build Info.plist:     ${infoplist}
Build reader.conf:    ${readerconf}
VPCD hostname: 	      ${vpcdhost}
VPCD slot count:      ${vpcdslots}

Host:                 ${host}
Compiler:             ${CC}
Preprocessor flags:   ${CPPFLAGS}
Compiler flags:       ${CFLAGS}
Linker flags:         ${LDFLAGS}
Libraries:            ${LIBS}

PTHREAD_LIBS:         ${PTHREAD_LIBS}
PTHREAD_CFLAGS:       ${PTHREAD_CFLAGS}
PCSC_CFLAGS:          ${PCSC_CFLAGS}
PCSC_LIBS:            ${PCSC_LIBS}
QRENCODE_CFLAGS:      ${QRENCODE_CFLAGS}
QRENCODE_LIBS:        ${QRENCODE_LIBS}
BUNDLE_HOST:          ${BUNDLE_HOST}
LIB_PREFIX:           ${LIB_PREFIX}
DYN_LIB_EXT:          ${DYN_LIB_EXT}

HELP2MAN:             ${HELP2MAN}
PYTHON:               ${PYTHON}

EOF
