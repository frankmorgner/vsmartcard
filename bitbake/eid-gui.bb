DESCRIPTION = "Simple GUI for entring a PIN and giving it to pace-tool"
LICENSE     = "GPL"
AUTHOR      = "Dominik Oepen <oepen@informatik.hu-berlin.de>"

DEPENDS     = "ccid-emulator"
RDEPENDS    = "ccid-emulator python-pygtk libglade"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=eID_gui;proto=https;rev=237"

S = "${WORKDIR}/eID_gui"

inherit autotools_stage pkgconfig python-dir

FILES_${PN} += "${bindir}/* \
                ${datadir}/eid_gui/* \
                ${PYTHON_SITEPACKAGES_DIR}/eid/*"
