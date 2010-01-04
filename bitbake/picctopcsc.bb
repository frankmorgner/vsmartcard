DESCRIPTION = "Virtual Smartcard with PCSC Driver and CCID to PCSC Gadget"
LICENSE     = "GPL"

DEPENDS     = "pcsc-lite"
RDEPENDS    = "pcsc-lite"

SRC_URI = "svn://svn.informatik.hu-berlin.de/svn/NFC;module=picc_to_pcsc;proto=https;rev=357"

S = "${WORKDIR}"

LIBPCSCLITE_CFLAGS = -I${STAGING_INCDIR}/PCSC -lpcsclite

INSTALL            = install
INSTALL_PROGRAM    = ${INSTALL}

FILES_${PN} = "\
    ${bindir}/* \
"

do_compile() {
    ${CC} ${S}/picc_to_pcsc/picc_to_pcsc.c -o ${S}/picc_to_pcsc/picc_to_pcsc \
        ${LIBPCSCLITE_CFLAGS} ${CFLAGS}
}

do_install() {
    ${INSTALL} -d ${D}${bindir} 
    ${INSTALL_PROGRAM} ${S}/picc_to_pcsc/picc_to_pcsc ${D}${bindir}
}
