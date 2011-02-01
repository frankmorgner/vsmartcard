DESCRIPTION = "A set of libraries and utilities to work with smart cards"
HOMEPAGE = "http://www.opensc-project.org/opensc"
LICENSE = "LGPL"

DEPENDS = "pcsc-lite openssl"
RDEPENDS = "libcrypto libpcsclite"

LEAD_SONAME = "libopensc"

SRC_URI = "http://www.opensc-project.org/files/opensc/opensc-${PV}.tar.gz;"

inherit autotools pkgconfig

EXTRA_OECONF = "--enable-pcsc=yes \
                --with-pcsc-provider=${libdir}/libpcsclite.so.1 \
               "

FILES_${PN} += "${libdir}/libopensc.so.*"

PACKAGES += "libpkcs11"
FILES_libpkcs11 = "${libdir}/pkcs11/*.so ${libdir}/pkcs11-spy.so ${libdir}/opensc-pkcs11.so ${libdir}/onepin-opensc-pkcs11.so"

SRC_URI[md5sum] = "630fa3b8717d22a1d069d120153a0c52"
SRC_URI[sha256sum] = "84f8a8e1825e487d321390f0650c590334c76f81291d2eb5a315ad73459d2f6f"
