.. highlight:: sh

.. _pcsc-relay:

###########
PC/SC Relay
###########

:Authors:
    - Frank Morgner <morgner@informatik.hu-berlin.de>
    - Dominik Oepen <oepen@informatik.hu-berlin.de>
:License:
    GPL version 3
:Tested Platforms:
    - Windows
    - Linux (Debian, Ubuntu, OpenMoko)
:Potential Platforms:
    Unix-like operating systems (Mac OS, Solaris, BSD, ...)

Welcome to PC/SC Relay. The purpose of PC/SC Relay is to relay a smart
card using an contact-less interface. Currently the following contact-less
backends:

- libnfc_
- OpenPICC_

Command APDUs are received with the contact-less interface and forwarded to an
existing smart card via PC/SC. The Response APDUs are then sent back via RFID.
You can use PC/SC Relay in combination with the :ref:`vicc` to completely
emulate an ISO/IEC 14443 smart card.

.. note::
    This software can actually be used in a relay attack allowing full access
    to the card. We discussed the impact especially on the `Relay attack
    against the German ID card`_, but it generally concerns *all* contact-less
    smart cards.


.. include:: download.txt


.. include:: autotools.txt

PC/SC Relay has the following dependencies:

- PC/SC middleware
- libnfc_

===============
Hints on libnfc
===============

PC/SC Relay links against libnfc_. Here is an example of how to get the standard
installation of the latter::
 
    PREFIX=/tmp/install
    LIBNFC=libnfc
    svn co http://libnfc.googlecode.com/svn/trunk $LIBNFC
    cd $LIBNFC
    autoreconf -i
    ./configure --prefix=$PREFIX
    make
    make install

Building PC/SC Relay with libnfc_ is done best using :command:`pkg-config`.  The file
:file:`libnfc.pc` should be located in ``$INSTALL/lib/pkgconfig``. Here is how to
configure PC/SC Relay to use it::

    ./configure PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig


=========================
Hints on PC/SC middleware
=========================

A PC/SC middleware is included by default in most modern operating systems. On
Unix-like systems (Linux, OS X, Sun OS) it is realized by PCSC-Lite_. To
compile PC/SC Relay you will need to install the PCSC-Lite_ headers from
your distribution.

Windows also ships with a PC/SC middleware in form of the Winscard module.
Microsoft's developement environment Visual Studio includes all necessary data
for building PC/SC Relay.


*****
Usage
*****

.. program-output:: pcsc-relay --help


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _libnfc: http://www.libnfc.org/
.. _OpenPICC: http://www.openpcd.org/OpenPICC
.. _PCSC-lite: http://pcsclite.alioth.debian.org/
.. _`Relay attack against the German ID card`: http://media.ccc.de/browse/congress/2010/27c3-4297-de-die_gesamte_technik_ist_sicher.html
