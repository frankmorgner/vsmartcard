DESCRIPTION = "API and Tools for the new German identity card (neuer Personalausweis, nPA)"
LICENSE     = "GPL"

DEPENDS     = "openssl opensc"
RDEPENDS    = "libcrypto"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot;module=vsmartcard;proto=https;rev=396"

S = "${WORKDIR}/vsmartcard/npa"

inherit autotools pkgconfig

EXTRA_OECONF = ""

FILES_${PN} += "\
    ${bindir}/* \
    ${libdir}/* \
"
