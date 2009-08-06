DESCRIPTION = "Virtual Smartcard with PCSC Driver and CCID to PCSC Gadget"
LICENSE     = "GPL"

DEPENDS     = "pcsc-lite linux-libc-headers"
RDEPENDS    = "pcsc-lite kernel-module-gadgetfs python-pycrypto python-crypt python-textutils python-imaging python-pickle"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/;module=vsmartcard;proto=https;rev=1"

S = "${WORKDIR}"
serialdropdir	   = ${libdir}/pcsc/drivers/serial
readerconfdir      = ${sysconfdir}/reader.conf.d
python_sitelib     = /usr/lib/python2.5/site-packages

LIBPCSCLITE_CFLAGS = -I${STAGING_INCDIR}/PCSC -lpcsclite
PTHREAD_CFLAGS     = -pthread
# FIXME: let openmoko set the right STAGING_KERNEL_DIR
STAGING_KERNEL_DIR = ${STAGING_DIR}/${MACHINE}-angstrom-${TARGET_OS}/kernel
GADGETFS_CFLAGS    = -I${STAGING_KERNEL_DIR}/include
SO_CFLAGS          = -fPIC
SO_LDFLAGS         = -shared

INSTALL            = install
INSTALL_PROGRAM    = ${INSTALL}
INSTALL_DATA       = ${INSTALL} -m 644

FILES_${PN} = "\
    ${bindir}/* \
    ${readerconfdir}/* \
    ${serialdropdir}/* \
    ${python_sitelib}/* \
"

do_compile() {
    ${CC} ${S}/NFC/ccid/ccid.c ${S}/NFC/ccid/usbstring.c -o ccid \
        ${LIBPCSCLITE_CFLAGS} ${GADGETFS_CFLAGS} ${PTHREAD_CFLAGS} ${CFLAGS}

    ${CC} ${S}/NFC/picc_to_pcsc/picc_to_pcsc.c -o picc_to_pcsc \
        ${LIBPCSCLITE_CFLAGS} ${CFLAGS}

    ${CC} -c ${S}/NFC/virtualsmartcard/vpcd/vpcd.c \
        ${S}/NFC/virtualsmartcard/vpcd/vpcd.h \
        ${LIBPCSCLITE_CFLAGS} ${SO_CFLAGS} ${CFLAGS}
    ${CC} vpcd.o -o libvpcd.so \
        ${LIBPCSCLITE_CFLAGS} ${SO_LDFLAGS} ${LDFLAGS}
    rm -f vpcd.o

    echo '#!/bin/sh' > virtualsmartcard
    echo "cd \"${python_sitelib}\"" \
        >> virtualsmartcard
    echo "python \"${python_sitelib}/VirtualSmartcard.py\" \"\$@\"" \
        >> virtualsmartcard

    echo 'FRIENDLYNAME "Virtual PCD"' > vpcd.conf
    echo 'DEVICENAME   /dev/null' >> vpcd.conf
    echo "LIBPATH      ${serialdropdir}/libvpcd.so" >> vpcd.conf
    echo 'CHANNELID    0' >> vpcd.conf
}

do_install() {
    ${INSTALL} -d ${D}${bindir} 
    ${INSTALL} -d ${D}${serialdropdir}
    ${INSTALL} -d ${D}${sysconfdir}/reader.conf.d/
    ${INSTALL} -d ${D}${python_sitelib}
    ${INSTALL_PROGRAM} ccid             ${D}${bindir}
    ${INSTALL_PROGRAM} picc_to_pcsc     ${D}${bindir}
    ${INSTALL_PROGRAM} virtualsmartcard ${D}${bindir}
    ${INSTALL_PROGRAM} libvpcd.so       ${D}${serialdropdir}
    ${INSTALL_DATA} vpcd.conf           ${D}${readerconfdir}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/ConstantDefinitions.py ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/CryptoUtils.py         ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/SEutils.py             ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/SmartcardFilesystem.py ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/SmartcardSAM.py        ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/SWutils.py             ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/TLVutils.py            ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/utils.py               ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/VirtualSmartcard.py    ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/testconfig.sam         ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/testconfig.mf          ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/NFC/virtualsmartcard/vpicc/jp2.jpg                ${D}${python_sitelib}
}
