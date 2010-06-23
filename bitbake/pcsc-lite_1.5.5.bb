DESCRIPTION = "PC/SC Lite smart card framework and applications"
HOMEPAGE = "http://pcsclite.alioth.debian.org/"
LICENSE = "BSD"

DEPENDS = "hal"
RDEPENDS_${PN} = "hal"

SRC_URI = "https://alioth.debian.org/frs/download.php/3082/pcsc-lite-${PV}.tar.bz2 \
           file://pcscd.init \
           file://pcsc-lite-1.5.5.patch;apply=yes \
          "

inherit autotools_stage update-rc.d

INITSCRIPT_NAME = "pcscd"
INITSCRIPT_PARAMS = "defaults"

EXTRA_OECONF = " \
	--enable-libhal \
	--disable-libusb \
	--enable-usbdropdir=${libdir}/pcsc/drivers \
	"

do_install() {
	oe_runmake DESTDIR="${D}" install
	install -d "${D}/etc/init.d"
	install -m 755 "${WORKDIR}/pcscd.init" "${D}/etc/init.d/pcscd"
}

PACKAGES =+ "libpcsclite"

FILES_libpcsclite = "${libdir}/libpcsclite.so.*"

SRC_URI[md5sum] = "6707e967fc8bb398a5d1b1089d4dff63"
SRC_URI[sha256sum] = "051de6f3c1deff9a9c6f72995f6b9d271a23fc8aea74737f1902cabf1a71ed26"
