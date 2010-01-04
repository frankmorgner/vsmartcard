DESCRIPTION = "Virtual Smartcard with PCSC Driver and CCID to PCSC Gadget"
LICENSE     = "GPL"

DEPENDS     = "pcsc-lite linux-libc-headers"
RDEPENDS    = "pcsc-lite kernel-module-gadgetfs"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard;module=ccid;proto=https;rev=1"

S = "${WORKDIR}"

LIBPCSCLITE_CFLAGS = -I${STAGING_INCDIR}/PCSC -lpcsclite
PTHREAD_CFLAGS     = -pthread
# FIXME: let openmoko set the right STAGING_KERNEL_DIR
STAGING_KERNEL_DIR = ${STAGING_DIR}/${MACHINE}-angstrom-${TARGET_OS}/kernel
GADGETFS_CFLAGS    = -I${STAGING_KERNEL_DIR}/include

INSTALL            = install
INSTALL_PROGRAM    = ${INSTALL}

FILES_${PN} = "\
    ${bindir}/* \
"

do_compile() {
    ${CC} ${S}/ccid/ccid.c ${S}/ccid/usbstring.c -o ${S}/ccid/ccid \
        ${LIBPCSCLITE_CFLAGS} ${GADGETFS_CFLAGS} ${PTHREAD_CFLAGS} ${CFLAGS}
}

do_install() {
    ${INSTALL} -d ${D}${bindir} 
    ${INSTALL_PROGRAM} ${S}/ccid/ccid ${D}${bindir}
}
