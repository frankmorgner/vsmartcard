DESCRIPTION = "A collection of cryptographic algorithms and protocols"
SECTION = "devel/python"
PRIORITY = "optional"
DEPENDS = "gmp"
SRCNAME = "pycrypto"
LICENSE = "pycrypto"
PR = "ml1"

SRC_URI = "http://www.amk.ca/files/python/crypto/${SRCNAME}-${PV}.tar.gz"
S = "${WORKDIR}/${SRCNAME}-${PV}"

inherit distutils

SRC_URI[md5sum] = "4d5674f3898a573691ffb335e8d749cd"
SRC_URI[sha256sum] = "b08d4ed54c9403c77778a3803e53a4f33f359b42d94f6f3e14abb1bf4941e6ea"
