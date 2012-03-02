SUMMARY = "Relay a smart card via NFC"
DESCRIPTION = "Forward APDUs from the OpenPICC or from a libnfc device to a smart card via the PC/SC middleware."
HOMEPAGE = "http://vsmartcard.sourceforge.net"
BUGTRACKER = "http://sourceforge.net/projects/vsmartcard/support"

LICENSE     = "GPL-3"
LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504"

DEPENDS     = "libnfc pcsc-lite"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot;module=vsmartcard;proto=https"
SRCREV = "719"
PV = "0.4+svnr${SRCPV}"
PR = "r0"

S = "${WORKDIR}/vsmartcard/pcsc-relay"

inherit autotools pkgconfig
