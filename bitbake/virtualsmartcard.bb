DESCRIPTION = "Virtual Smartcard with PCSC Driver and CCID to PCSC Gadget"
LICENSE     = "GPL"

DEPENDS     = "pcsc-lite"
RDEPENDS    = "pcsc-lite python-pycrypto python-crypt python-textutils python-imaging python-pickle"

SRC_URI = "svn://vsmartcard.svn.sourceforge.net/svnroot/vsmartcard/;module=virtualsmartcard;proto=https;rev=1"

S = "${WORKDIR}"
serialdropdir	   = ${libdir}/pcsc/drivers/serial
readerconfdir      = ${sysconfdir}/reader.conf.d
python_sitelib     = /usr/lib/python2.5/site-packages

LIBPCSCLITE_CFLAGS = -I${STAGING_INCDIR}/PCSC -lpcsclite
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
    ${CC} ${SO_CFLAGS} ${CFLAGS} -c \
        ${S}/virtualsmartcard/vpcd/vpcd.c \
        ${S}/virtualsmartcard/vpcd/vpcd.h \
    ${CC} ${SO_CFLAGS} ${CFLAGS} ${LIBPCSCLITE_CFLAGS} -c \
        ${S}/virtualsmartcard/vpcd/ifd.c
        
    ${CC} ${S}/ifd.o ${S}/vpcd.o -o ${S}/virtualsmartcard/libvpcd.so \
        ${LIBPCSCLITE_CFLAGS} ${SO_LDFLAGS} ${LDFLAGS}
    rm -f ${S}/vpcd.o ${S}/ifd.o

    echo '#!/bin/sh' > ${S}/virtualsmartcard/virtualsmartcard
    echo "cd \"${python_sitelib}\"" \
        >> ${S}/virtualsmartcard/virtualsmartcard
    echo "python \"${python_sitelib}/VirtualSmartcard.py\" \"\$@\"" \
        >> ${S}/virtualsmartcard/virtualsmartcard

    echo 'FRIENDLYNAME "Virtual PCD"' > ${S}/virtualsmartcard/vpcd.conf
    echo 'DEVICENAME   /dev/null' >> ${S}/virtualsmartcard/vpcd.conf
    echo "LIBPATH      ${serialdropdir}/libvpcd.so" >> ${S}/virtualsmartcard/vpcd.conf
    echo 'CHANNELID    0' >> ${S}/virtualsmartcard/vpcd.conf
}

do_install() {
    ${INSTALL} -d ${D}${bindir} 
    ${INSTALL} -d ${D}${serialdropdir}
    ${INSTALL} -d ${D}${readerconfdir}
    ${INSTALL} -d ${D}${python_sitelib}
    ${INSTALL_PROGRAM} ${S}/virtualsmartcard/virtualsmartcard ${D}${bindir}
    ${INSTALL_PROGRAM} ${S}/virtualsmartcard/libvpcd.so       ${D}${serialdropdir}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpcd.conf           ${D}${readerconfdir}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/ConstantDefinitions.py ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/CryptoUtils.py         ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/SEutils.py             ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/SmartcardFilesystem.py ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/SmartcardSAM.py        ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/SWutils.py             ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/TLVutils.py            ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/utils.py               ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/VirtualSmartcard.py    ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/testconfig.sam         ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/testconfig.mf          ${D}${python_sitelib}
    ${INSTALL_DATA} ${S}/virtualsmartcard/vpicc/jp2.jpg                ${D}${python_sitelib}
}
