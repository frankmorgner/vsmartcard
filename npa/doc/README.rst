.. highlight:: sh

.. _OpenSC: http://www.opensc-project.org
.. _OpenSC with PACE: http://github.com/frankmorgner/OpenSC
.. _OpenSSL: http://www.openssl.org
.. _OpenPACE: http://openpace.sourceforge.net

.. |PACE| replace:: :abbr:`PACE (Password Authenticated Connection Establishment)`
.. |npa-tool| replace:: :command:`npa-tool`


.. _npa:

********************************************************************************
nPA Smart Card Library
********************************************************************************

:Author:
    Frank Morgner <morgner@informatik.hu-berlin.de>
:License:
    GPL version 3
:Tested Platforms:
    - Windows
    - Linux (Debian, Ubuntu, OpenMoko)
:Potential Platforms:
    Unix-like operating systems (Mac OS, Solaris, BSD, ...)

The nPA Smart Card Library offers an easy to use API for the new German identity card
(neuer Personalausweis, nPA). The library also implements secure messaging,
which could also be used for other cards. The included |npa-tool| can
be used for PIN management or to send APDUs inside a secure channel.

The nPA Smart Card Library is implemented using OpenPACE_.  Some fragments of the
source code are based on the source code of the OpenSC_ tools.


.. _npa-install:

.. include:: autotools.rst

The nPA Smart Card Library has the following dependencies:

- `OpenSC with PACE`_
- OpenSSL_ with OpenPACE_


------------------------------
Hints on OpenSSL with OpenPACE
------------------------------

The nPA Smart Card Library links against OpenSSL_, which must be patched with
OpenPACE_.  Here is an example of how to get the standard installation of
OpenPACE_::
 
    PREFIX=/tmp/install
    OPENPACE=openpace
    svn co https://openpace.svn.sourceforge.net/svnroot/openpace $OPENPACE
    cd $OPENPACE
    make patch_with_openpace
    cd openpace
    ./config experimental-pace --prefix=$PREFIX
    make depend
    make
    make install

Building the nPA Smart Card Library with OpenPACE_ is done best using
:command:`pkg-config`.  The file :file:`libcrypto.pc` should be located in
``$INSTALL/lib/pkgconfig``. Here is how to configure the nPA Smart Card Library to use
it::

    ./configure PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig


---------------
Hints on OpenSC
---------------

The nPA Smart Card Library links against OpenSC, which is discouraged and hindered
since OpenSC version >= 0.12. However, I extended OpenSC to support smart card
readers with PACE capabilities. You need the OpenSC components to be installed
(especially :file:`libopensc.so`). Here is an example of how to get the
standard installation of `OpenSC with PACE`_::

    PREFIX=/tmp/install
    OPENSC=opensc
    git clone git://github.com/frankmorgner/OpenSC.git $OPENSC
    cd $OPENSC
    autoreconf -i
    # adding PKG_CONFIG_PATH here lets OpenSC use OpenSSL with OpenPACE
    ./configure --prefix=$PREFIX PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
    make
    make install

Now :file:`libopensc.so` should be located in ``$PREFIX/lib``. Here is how to
configure the nPA Smart Card Library to use it::

    ./configure OPENSC_LIBS="-L$PREFIX/lib -lopensc"


.. _npa-usage:

=====
Usage
=====

To pass a secret to |npa-tool| for |PACE|, command line parameters or
environment variables can be used. If the smart card reader supports |PACE|,
the PIN pad is used. If none of these options is applies, npa-tool will show a
password prompt.

|npa-tool| can send arbitrary APDUs to the nPA in the secure channel.  APDUs
are entered interactively or through a file.  APDUs are formatted in hex (upper
or lower case) with an optional colon to separate the bytes. Example APDUs can
be found in :file:`apdus`.

.. program-output:: npa-tool --help

----------------------
Linking against libnpa
----------------------

Following the section `Installation`_ above, you have installed `OpenSC with
PACE`_, OpenPACE_ and the nPA Smart Card Library to :file:`/tmp/install`. To compile a
program using nPA Smart Card Library you need to get the header files from `OpenSC with
PACE`_ as well.  Here is how to compile an external program with these
libraries::

    PREFIX=/tmp/install
    OPENSC=opensc
    git clone git://github.com/frankmorgner/OpenSC.git $OPENSC
    cc example.c -I$OPENSC/src \
        $(env PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig \
            pkg-config --cflags --libs npa)

Alternatively you can specify libraries and flags by hand::

    PREFIX=/tmp/install
    OPENSC=opensc
    git clone git://github.com/frankmorgner/OpenSC.git $OPENSC
    cc example.c -I$OPENSC/src \
        -I$PREFIX/include \
        -L$PREFIX/lib -lcrypto -lnpa -lopensc"


.. include:: questions.rst
