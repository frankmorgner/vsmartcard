DESCRIPTION = "Virtual Smartcard with PCSC Driver and CCID to PCSC Gadget"
LICENSE     = "GPL"

DEPENDS     = "pcsc-lite linux-libc-headers openssl opensc"
RDEPENDS    = "pcsc-lite kernel-module-gadgetfs libcrypto opensc"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=ccid;proto=https;rev=417"

S = "${WORKDIR}/ccid"

inherit autotools pkgconfig

EXTRA_OECONF = "--enable-pace"

FILES_${PN} += "\
    ${bindir}/* \
"
