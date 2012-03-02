SUMMARY = "Virtual Smart Card"
DESCRIPTION = "Virtual smart card with PC/SC Driver"
HOMEPAGE = "http://vsmartcard.sourceforge.net"
BUGTRACKER = "http://sourceforge.net/projects/vsmartcard/support"

LICENSE     = "GPL-3"
LIC_FILES_CHKSUM = "file://COPYING;md5=d32239bcb673463ab874e80d47fae504"

DEPENDS     = "pcsc-lite"
RDEPENDS   += "python-pycrypto python-crypt python-textutils python-imaging python-pickle"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot;module=vsmartcard;proto=https"
SRCREV = "719"
PV = "0.6+svnr${SRCPV}"
PR = "r0"

S = "${WORKDIR}/vsmartcard/virtualsmartcard"

EXTRA_OECONF = "--enable-serialdropdir=${libdir}/pcsc/drivers/serial"

inherit autotools pkgconfig

FILES_${PN} += "\
    ${libdir}/python2.7 \
    ${libdir}/pcsc/drivers/serial/libvpcd.so.0.5 \
"
FILES_${PN}-dbg += "${libdir}/pcsc/drivers/serial/.debug"
FILES_${PN}-dev += "${libdir}/pcsc/drivers/serial/libvpcd.so"
