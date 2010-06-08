DESCRIPTION = "Simple GUI for entring a PIN and giving it to pace-tool"
LICENSE     = "GPL"

DEPENDS     = "ccid-tool"
RDEPENDS    = "ccid-tool python-pygtk libglade"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=pace-gui;proto=https;rev=137"

S = "${WORKDIR}/pace-gui"

inherit autotools_stage pkgconfig

FILES_${PN} += "${bindir}/* \
                ${datadir}/pinpad/*"
