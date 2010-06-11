DESCRIPTION = "Simple GUI for entring a PIN and giving it to pace-tool"
LICENSE     = "GPL"

DEPENDS     = "ccid-emulator"
RDEPENDS    = "ccid-emulator python-pygtk libglade"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=pace-gui;proto=https;rev=145"

S = "${WORKDIR}/pace-gui"

inherit autotools_stage pkgconfig

FILES_${PN} += "${bindir}/* \
                ${datadir}/pinpad/*"
