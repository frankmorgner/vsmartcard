DESCRIPTION = "Generic USB CCID smart card reader driver"
HOMEPAGE = "http://pcsclite.alioth.debian.org/ccid.html"
LICENSE = "GPL"
PR = "r0"

DEPENDS = "virtual/libusb0 pcsc-lite"
RDEPENDS = "pcsc-lite"

SRC_URI = "http://alioth.debian.org/download.php/3080/ccid-${PV}.tar.bz2 \
           file://ccid-1.3.11.patch;apply=yes \
          "

inherit autotools

EXTRA_OECONF = "--enable-udev"

do_install_append () {
	install -d "${D}/etc/udev/rules.d"
	install -m 644 "${S}/src/pcscd_ccid.rules" "${D}/etc/udev/rules.d/85-pcscd_ccid.rules"
}

FILES_${PN} += "${libdir}/pcsc/"
FILES_${PN}-dbg += "${libdir}/pcsc/drivers/*/*/*/.debug"

SRC_URI[md5sum] = "727dc7eb4d560f81fe70a766a96de970"
SRC_URI[sha256sum] = "f7bf5a82b02aff7d709a45dcc6109105508625caf92e34da4140b7c84d498906"
