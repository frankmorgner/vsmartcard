.. highlight:: sh

.. _OpenSC: http://www.opensc-project.org/opensc
.. _OpenPACE: http://sourceforge.net/projects/openpace/


.. _npa:

***
npa
***

:Author:
    Frank Morgner <morgner@informatik.hu-berlin.de>
:License:
    GPL version 3
:Tested Platforms:
    Linux (Debian, Ubuntu, OpenMoko)

Welcome to npa. The purpose of npa is to offer an easy to use API for the new
German identity card (neuer Personalausweis, nPA). The library also implements
secure messaging, which could also be used for other cards.

npa is implemented using OpenPACE_.
Some fragments of the source code are based on the source code of the OpenSC tools.

The included npa-tool has support for Password Authenticated Connection
Establishment (PACE). npa-tool can be used for PIN management or to encrypt
APDUs inside a secure messaging channel established with PACE.


.. _npa-install:

.. include:: autotools.rst

npa has the following dependencies:

- OpenSC_
- OpenSSL with OpenPACE_


------------------------------
Hints on OpenSSL with OpenPACE
------------------------------

libnpa links against libcrypto, which must be patched with OpenPACE_. Here is
an example of how to get the standard installation of OpenSSL with OpenPACE_::
 
    PREFIX=/tmp/install
    OPENPACE=openpace
    svn co https://openpace.svn.sourceforge.net/svnroot/openpace $OPENPACE
    cd $OPENPACE
    make patch_with_openpace
    cd openssl-*/
    ./config experimental-pace --prefix=$PREFIX
    make depend
    make
    make install

Building npa with OpenPACE_ is done best using :command:`pkg-config`.  The file
:file:`libcrypto.pc` should be located in ``$INSTALL/lib/pkgconfig``. Here is how
to configure npa to use it::

    ./configure PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig


---------------
Hints on OpenSC
---------------

libnpa links against libopensc, which is discouraged and hindered since OpenSC
version >= 0.12. (We really need to get rid of this dependency or integrate
better into the OpenSC-framework.) You need the OpenSC components to be
installed (especially :file:`libopensc.so`). Here is an example of how to get the
standard installation of OpenSC_::

    PREFIX=/tmp/install
    OPENSC=opensc
    svn co http://www.opensc-project.org/svn/opensc/trunk $OPENSC
    cd $OPENSC
    autoreconf -i
    # adding PKG_CONFIG_PATH here lets OpenSC use OpenSSL with OpenPACE
    ./configure --prefix=$PREFIX PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
    make
    make install

Now :file:`libopensc.so` should be located in ``$PREFIX/lib``. Here is how to
configure npa to use it::

    ./configure OPENSC_LIBS="-L$PREFIX/lib -lopensc"


.. _npa-usage:

=====
Usage
=====

When testing PACE with either PIN, CAN, MRZ or PUK run npa-tool. Here you can
enter APDUs which are to be converted according to the secure messaging
parameter and to be sent to the card. Herefor insert the APDU in hex (upper or
lower case) with a colon to separate the bytes or without it. Example APDUs can
be found in the file apdus.

To pass a secret to npa-tool, the command line parameters or the environment
variables PIN/CAN/MRZ/PUK/NEWPIN can be used. If none of these options is used,
npa-tool will show a password prompt.

----------------------
Linking against libnpa
----------------------

Following the section `Installation`_ above, you have installed OpenSC_,
OpenPACE_ and npa to :file:`/tmp/install`. To compile a program using libnpa you
need to get the header files from OpenSC_ as well.
Here is how
to compile an external program with these libraries::

    PREFIX=/tmp/install
    OPENSC=opensc
    svn co http://www.opensc-project.org/svn/opensc/trunk $OPENSC
    cc example.c -I$OPENSC/src \
        $(env PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig \
            pkg-config --cflags --libs npa)

Alternatively you can specify libraries and flags by hand::

    PREFIX=/tmp/install
    OPENSC=opensc
    svn co http://www.opensc-project.org/svn/opensc/trunk $OPENSC
    cc example.c -I$OPENSC/src \
        -I$PREFIX/include \
        -L$PREFIX/lib -lcrypto -lnpa -lopensc"


.. include:: questions.rst
