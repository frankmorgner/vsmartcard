DESCRIPTION = "Virtual Smartcard with PCSC Driver and CCID to PCSC Gadget"
LICENSE     = "GPL"

DEPENDS     = "pcsc-lite"
RDEPENDS    = "pcsc-lite python-pycrypto python-crypt python-textutils python-imaging python-pickle"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=virtualsmartcard;proto=https;rev=131"

S = "${WORKDIR}/virtualsmartcard"

inherit autotools_stage pkgconfig

FILES_${PN} += "\
    ${bindir}/* \
    ${sysconfdir}/* \
    ${libdir}/* \
"
