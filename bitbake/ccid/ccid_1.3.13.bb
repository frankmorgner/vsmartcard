DESCRIPTION = "Generic USB CCID smart card reader driver"
HOMEPAGE = "http://pcsclite.alioth.debian.org/ccid.html"
LICENSE = "GPL"
PR = "r0"

DEPENDS = "virtual/libusb1 pcsc-lite"
RDEPENDS = "pcsc-lite"

SRC_URI = "http://alioth.debian.org/download.php/3300/ccid-${PV}.tar.bz2 \
           file://ccid-1.3.13.patch;apply=yes"

inherit autotools

EXTRA_OECONF = "--enable-udev"

do_install_append () {
	install -d "${D}/etc/udev/rules.d"
	install -m 644 "${S}/src/pcscd_ccid.rules" "${D}/etc/udev/rules.d/85-pcscd_ccid.rules"
}

FILES_${PN} += "${libdir}/pcsc/"
FILES_${PN}-dbg += "${libdir}/pcsc/drivers/*/*/*/.debug"

SRC_URI[md5sum] = "275360cb253299b763e1122adf847265"
SRC_URI[sha256sum] = "f797b08874c1f9b2b4afe4ada8ef3400ce4da6bf965a24a6cf7f29d82cbf8c53"
