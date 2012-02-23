DESCRIPTION = "Generic USB CCID smart card reader driver"
HOMEPAGE = "http://pcsclite.alioth.debian.org/ccid.html"
LICENSE = "LGPLv2.1+"
LIC_FILES_CHKSUM = "file://COPYING;md5=2d5025d4aa3495befef8f17206a5b0a1"
PR = "r0"

DEPENDS = "virtual/libusb0 pcsc-lite"
RDEPENDS_${PN} = "pcsc-lite"

SRC_URI = "https://alioth.debian.org/frs/download.php/3579/ccid-${PV}.tar.bz2 \
           file://ccid-1.4.4.patch;apply=yes"

SRC_URI[md5sum] = "79ef91103bcdd99a3b31cb5c5721a829"
SRC_URI[sha256sum] = "953e430d2e37a67b99041f584249085656d73e72390dfe40589f4dd5c367edd0"

inherit autotools

FILES_${PN} += "${libdir}/pcsc/"
FILES_${PN}-dbg += "${libdir}/pcsc/drivers/*/*/*/.debug"
