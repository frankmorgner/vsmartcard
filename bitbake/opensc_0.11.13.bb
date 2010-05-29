DESCRIPTION = "A set of libraries and utilities to work with smart cards"
HOMEPAGE = "http://www.opensc-project.org/opensc"
LICENSE = "LGPL"

DEPENDS = "openssl"
RDEPENDS = "libcrypto"

LEAD_SONAME = "libopensc"

SRC_URI = "http://www.opensc-project.org/files/opensc/opensc-${PV}.tar.gz;"

inherit autotools_stage pkgconfig

SRC_URI += "file://le0.patch"

FILES_${PN} += "${libdir}/libopensc.so.*"

PACKAGES += "libpkcs11"
FILES_libpkcs11 = "${libdir}/pkcs11/*.so ${libdir}/pkcs11-spy.so ${libdir}/opensc-pkcs11.so ${libdir}/onepin-opensc-pkcs11.so"

SRC_URI[md5sum] = "98fa151e947941f9c3f27420fdf47c11"
SRC_URI[sha256sum] = "a9a42d6d51fb500f34248fcd0d4083c99d25bc5e74df60fe4efa19b5b4e6d890"
