DESCRIPTION = "Virtual Smartcard with PCSC Driver and CCID to PCSC Gadget"
LICENSE     = "GPL"

DEPENDS     = "pcsc-lite"
RDEPENDS    = "pcsc-lite python-pycrypto python-crypt python-textutils python-imaging python-pickle"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=virtualsmartcard;proto=https;rev=164"

S = "${WORKDIR}/virtualsmartcard"

EXTRA_OECONF = " --enable-serialdropdir=${libdir}/pcsc/drivers/serial"

inherit autotools_stage pkgconfig

FILES_${PN} += "\
    ${bindir}/* \
    ${sysconfdir}/* \
    ${libdir}/* \
"
