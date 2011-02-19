DESCRIPTION = "Simple GUI for entring a PIN and giving it to npa-tool"
LICENSE     = "GPL"
AUTHOR      = "Dominik Oepen <oepen@informatik.hu-berlin.de>"

DEPENDS     = "npa"
RDEPENDS    = "npa python-pygtk libglade"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=eID_gui;proto=https;rev=417"

S = "${WORKDIR}/eID_gui"

inherit autotools pkgconfig python-dir

FILES_${PN} += "${bindir}/* \
                ${datadir}/eid_gui/* \
                ${PYTHON_SITEPACKAGES_DIR}/eid/*"
