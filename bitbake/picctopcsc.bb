DESCRIPTION = "Virtual Smartcard with PCSC Driver and CCID to PCSC Gadget"
LICENSE     = "GPL"

DEPENDS     = "pcsc-lite"
RDEPENDS    = "pcsc-lite"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=picc_to_pcsc;proto=https;rev=131"

S = "${WORKDIR}/picc_to_pcsc"

inherit autotools_stage pkgconfig

FILES_${PN} += "\
    ${bindir}/* \
"
