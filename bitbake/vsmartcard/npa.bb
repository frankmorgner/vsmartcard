SUMMARY = "nPA Smart Card Library"
DESCRIPTION = "API and Tool for the new German identity card (neuer Personalausweis, nPA)"
HOMEPAGE = "http://vsmartcard.sourceforge.net"
BUGTRACKER = "http://sourceforge.net/projects/vsmartcard/support"

LICENSE     = "GPL-3"
LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504"

DEPENDS     = "opensc openssl"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot;module=vsmartcard;proto=https"
SRCREV = "719"
PV = "0.4+svnr${SRCPV}"
PR = "r2"

# statically link to openssl
EXTRA_OECONF += 'OPENSSL_LIBS="${STAGING_DIR_TARGET}/lib/libcrypto.a -ldl"'

S = "${WORKDIR}/vsmartcard/npa"

inherit autotools pkgconfig
