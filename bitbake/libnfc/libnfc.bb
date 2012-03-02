SUMMARY = "Public platform independent Near Field Communication (NFC) library"
DESCRIPTION = "libnfc is a library which allows userspace application access to NFC devices."
HOMEPAGE = "http://www.libnfc.org/"
BUGTRACKER = "http://code.google.com/p/libnfc/issues/list"

LICENSE     = "LGPLv3"
LIC_FILES_CHKSUM = "file://COPYING;md5=b52f2d57d10c4f7ee67a7eb9615d5d24"

SRC_URI = "svn://libnfc.googlecode.com/svn/tags/;module=libnfc-1.5.1;proto=http"
SRCREV = "1326"
PV = "1.5.1+svnr${SRCPV}"
PR = "r0"

S = "${WORKDIR}/libnfc-1.5.1"

inherit autotools pkgconfig
