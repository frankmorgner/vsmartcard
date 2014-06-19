.. highlight:: sh

.. |PACE| replace:: :abbr:`PACE (Password Authenticated Connection Establishment)`
.. |TA| replace:: :abbr:`TA (Terminal Authenticatation)`
.. |CA| replace:: :abbr:`CA (Chip Authentication)`
.. |EAC| replace:: :abbr:`EAC (Extended Access Control)`
.. |CSCA| replace:: :abbr:`CSCA (Country Signing Certificate Authority)`
.. |npa-tool| replace:: :command:`npa-tool`

.. _libnpa:

################################################################################
nPA Smart Card Library
################################################################################

.. sidebar:: Summary

    Access the German electronic identity card (neuer Personalausweis/nPA)

    :Author:
        `Frank Morgner <morgner@informatik.hu-berlin.de>`_
    :License:
        GPL version 3
    :Tested Platforms:
        - Windows
        - Linux (Debian, Ubuntu, OpenMoko)

The nPA Smart Card Library offers an easy to use API for the new German identity card
(neuer Personalausweis, nPA). The library also implements secure messaging,
which could also be used for other cards. The included |npa-tool| can
be used for PIN management or to send APDUs inside a secure channel.

The nPA Smart Card Library is implemented using OpenPACE_ and OpenSC_. nPA Smart Card Library
implements and initializes Secure Messaging wrappers of OpenSC to allow a
transparent SM usage in OpenSC. This allows nPA Smart Card Library to be fully
compatible with OpenSC.

.. tikz:: Architecture of the nPA Smart Card Library
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{%(wd)s/bilder/tikzstyles.tex}
    \node (npa-tool) [aktivbox] {\texttt{npa-tool}};
    \node (libnpa) [box, below=of npa-tool] {\texttt{libnpa}};
	\node (opensc)
	[klein, box, shape=rectangle split, rectangle split parts=2, right=of libnpa,
    kleiner, yshift=-1cm]
	{OpenSC (\texttt{libopensc})
	\nodepart{second}
	\footnotesize PC/SC\qquad CT-API
	};
	\draw [box] ($(opensc.text split)-(.05cm,0)$) -- ($(opensc.south)-(.05cm,0)$);
	\node (openpace) [klein, box, left=of libnpa, yshift=-1cm] {OpenPACE};

    \begin{pgfonlayer}{background}
        \path[linie]
        (npa-tool) edge (libnpa)
        (libnpa) edge (opensc)
        (libnpa) edge (openpace);
    \end{pgfonlayer}


.. include:: download.txt


.. _npa-install:

.. include:: autotools.txt

The nPA Smart Card Library has the following dependencies:

- OpenPACE_
- OpenSC_
- OpenSSL_


====================================
Installation of OpenPACE and OpenSSL
====================================

The nPA Smart Card Library links against OpenSSL, which must be patched for OpenPACE.
Here is an example of how to get the standard installation of OpenPACE (with
the required binaries for OpenSSL)::
 
    PREFIX=/tmp/install
    OPENPACE=$PWD/openpace
    git clone https://github.com/frankmorgner/openpace.git $OPENPACE
    cd $OPENPACE
    autoreconf --verbose --install
    # with `--enable-openssl-install` OpenSSL will be downloaded and installed along with OpenPACE
    ./configure --enable-openssl-install --prefix=$PREFIX
    make install && cd -

The file :file:`libcrypto.pc` should be located in ``$INSTALL/lib/pkgconfig``.


======================
Installation of OpenSC
======================

The nPA Smart Card Library needs the OpenSC components to be installed (especially
:file:`libopensc.so`). Here is an example of how to get a suitable installation
of OpenSC::

    VSMARTCARD=$PWD/vsmartcard
    git clone https://github.com/frankmorgner/vsmartcard.git $VSMARTCARD
    cd $VSMARTCARD
    git submodule init
    git submodule update
    cd $VSMARTCARD/npa/src/opensc
    autoreconf --verbose --install
    # adding PKG_CONFIG_PATH here lets OpenSC use the patched OpenSSL
    ./configure --prefix=$PREFIX PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig --enable-sm
    make install && cd -

Now :file:`libopensc.so` should be located in ``$PREFIX/lib``.


================================================================================
Installation of the nPA Smart Card Library
================================================================================

To complete this step-by-step guide, here is how to install nPA Smart Card Library::

    cd $VSMARTCARD/npa
    autoreconf --verbose --install
    ./configure --prefix=$PREFIX PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig OPENSC_LIBS="-L$PREFIX/lib -lopensc -lcrypto"
    make install && cd -



.. _npa-usage:

********************************************************************************
Using the nPA Smart Card Library
********************************************************************************

The API to libnpa is documented in :ref:`npa-api`. It includes a simple
programming example. Here we will focus on the command line interface to the
library offered by the |npa-tool|. It can perform |EAC| (i.e. |PACE|, |TA|,
|CA|) and read data groups from the identity card.

To pass a secret to |npa-tool| for |PACE|, command line parameters or
environment variables can be used. If the smart card reader supports |PACE|,
its PIN pad is used. If none of these options apply, |npa-tool| will show a
password prompt.

The certificates certificate chain for |TA| should be passed in the correct
order (finishing with the terminal certificate) so that the card can verify it.
|CA| is always done when the terminal's signature has been verified
successfully. The appropriate |CSCA| certificate will automatically be looked
up by OpenPACE.

|npa-tool| can send arbitrary APDUs to the nPA in the secure channel (after
|PACE| or |EAC|).  APDUs are entered interactively or through a file.  APDUs
are formatted in hex (upper or lower case) with an optional colon to separate
the bytes. Example APDUs can be found in :file:`apdus`.

.. program-output:: npa-tool --help


======================
Linking against libnpa
======================

Following the section Installation_ above, you have installed OpenSSL,
OpenPACE, OpenSC and the nPA Smart Card Library to `$PREFIX` which points to
:file:`/tmp/install`. To compile a program using nPA Smart Card Library you also need
the OpenSC header files, which are located in
:file:`$VSMARTCARD/npa/src/opensc` Here is how to compile an external program
with these libraries::

    PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
    cc example.c -I$VSMARTCARD/npa/src/opensc \
        $(pkg-config --cflags --libs npa)

Alternatively you can specify libraries and flags by hand::

    cc example.c -I$VSMARTCARD/npa/src/opensc \
        -I$PREFIX/include \
        -L$PREFIX/lib -lnpa -lopensc -lcrypto"


********************************************************************************
Using the German identity card with OpenSC
********************************************************************************

.. versionadded:: 0.6
    Added external card driver and PKCS#15 emulator for supporting nPA in
    OpenSC.

To let OpenSC recognize the German ID card we implemented an external card
driver. We supply a sample :file:`opensc.conf` which adds our driver to all
components of OpenSC. Load it by setting :envvar:`OPENSC_CONF`::

    export OPENSC_CONF=$VSMARTCARD/npa/opensc.conf

On Windows you need to use :command:`set` instead of :command:`export`. In
:file:`npa-0.6_win32` do the following::

    cd bin
    set OPENSC_CONF=..\etc\opensc.conf

The card driver recognizes the PIN verification method by it's ID. MRZ, CAN,
eID-PIN and PUK are verified using |PACE| (ID ``0x01``, ``0x02``, ``0x03`` and
``0x04``). The eSign PIN (ID ``0x83``) is verified using a standard ISO-7816-4
VERIFY command. Here, for example, we show how to verify the eID-PIN "123456"
with :command:`opensc-explorer`::

    opensc-explorer
    OpenSC [3F00]> verify CHV3 313233343536
    Code correct.

Since the German ID card does not contain any PKCS#15 data we emulate the data
structures with an other external library. Currently, the PKCS#15 emulator does
not support private key and certificate objects for qualified electronic
signature but only the PIN objects. For example, you can change the eID-PIN
using the :command:`pkcs15-tool`::

    pkcs15-tool --change-pin --auth-id 03 \
        --pin=123456 --new-pin=abcdef     # yes, an ASCII eID-PIN is allowed

When the eID-PIN was verified incorrectly three times, it is blocked and must
be unblocked with the PUK. But unlike traditional cards the German ID card does
suspend the eID-PIN after the second try of verification. To unlock the last
retry, the CAN is required.  Since the suspended state is not captured by
OpenSC we handle it transparently within the driver. If the eID-PIN shall be
verified and it is suspended, the card driver will verify the CAN first. If no
CAN is given in :file:`opensc.conf`, the driver will request it on the standard
input.


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _OpenPACE: https://frankmorgner.github.io/vsmartcard/
.. _OpenSC: https://github.com/OpenSC/OpenSC
.. _OpenSSL: http://www.openssl.org
