DESCRIPTION = "PC/SC Lite smart card framework and applications"
HOMEPAGE = "http://pcsclite.alioth.debian.org/"
LICENSE = "BSD"

DEPENDS = "hal"
RDEPENDS_${PN} = "hal"

SRC_URI = "https://alioth.debian.org/frs/download.php/3279/pcsc-lite-${PV}.tar.bz2 \
           file://pcscd.init "

inherit autotools_stage update-rc.d

INITSCRIPT_NAME = "pcscd"
INITSCRIPT_PARAMS = "defaults"

EXTRA_OECONF = " \
	--enable-libusb \
	--disable-libhal \
	--enable-usbdropdir=${libdir}/pcsc/drivers \
	"

do_install() {
	oe_runmake DESTDIR="${D}" install
	install -d "${D}/etc/init.d"
	install -m 755 "${WORKDIR}/pcscd.init" "${D}/etc/init.d/pcscd"
}

PACKAGES =+ "libpcsclite"

FILES_libpcsclite = "${libdir}/libpcsclite.so.*"

SRC_URI[md5sum] = "fc3fd0e83090ecc81e5b32700fa246c2"
SRC_URI[sha256sum] = "4d3f308e8a678315592628ec121061073fa2164c2710d964c14d9e9aa8fe8968"
