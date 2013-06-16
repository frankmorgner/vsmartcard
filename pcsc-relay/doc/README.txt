.. highlight:: sh

.. _pcsc-relay:

################################################################################
PC/SC Relay
################################################################################

.. sidebar:: Summary

  Relay a smart card to a contactless interface

  :Authors:
      - `Frank Morgner <morgner@informatik.hu-berlin.de>`_
      - `Dominik Oepen <oepen@informatik.hu-berlin.de>`_
  :License:
      GPL version 3
  :Tested Platforms:
      - Windows
      - Linux (Debian, Ubuntu, OpenMoko)

Welcome to PC/SC Relay. The purpose of PC/SC Relay is to relay a smart
card using an contact-less interface. Currently the following contact-less
backends are supported:

- `Hardware supported by libnfc`_
- OpenPICC_

Command APDUs are received with the contact-less interface and relayed. The
Response APDUs are then sent back via RFID. The contact-less data will be
relayed to one of the following:

- to a smart card inserted into one of the systems smart card readers. The
  smart card reader must be accessible with PC/SC.
- to a :ref:`vicc` that directly connects to :command:`pcsc-relay`. The virtual
  smart card's native interface is used and (despite its name) PC/SC Relay
  does not need to access PC/SC in this case.

With PC/SC Relay you can relay a contact-less or contact based smart card
over a long distance. Also you can use it in combination with the :ref:`vicc`
to completely emulate an ISO/IEC 14443 smart card.



.. include:: relay-note.txt


.. include:: download.txt


.. include:: autotools.txt

PC/SC Relay has the following dependencies:

- PC/SC middleware
- libnfc_

===============
Hints on libnfc
===============

Here is an example of how to get the standard
installation of libnfc::
 
    PREFIX=/tmp/install
    LIBNFC=libnfc
    git clone https://code.google.com/p/libnfc $LIBNFC
    cd $LIBNFC
    autoreconf -i
    # See `./configure --help` for enabling support of additional hardware
    ./configure --prefix=$PREFIX
    make
    make install

Building PC/SC Relay with libnfc is done best using :command:`pkg-config`.  The file
:file:`libnfc.pc` should be located in ``$INSTALL/lib/pkgconfig``. Here is how to
configure PC/SC Relay to use it::

    ./configure PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig


=========================
Hints on PC/SC middleware
=========================

A PC/SC middleware is included by default in most modern operating systems. On
Unix-like systems (Linux, OS X, Sun OS) it is realized by PCSC-Lite_. To
compile PC/SC Relay you will need to install the PCSC-Lite headers from
your distribution.

Windows also ships with a PC/SC middleware in form of the Winscard module.
PC/SC Relay can be (cross) compiled with MinGW-w64. Also, Microsoft's
developement environment Visual Studio includes all necessary data for building
PC/SC Relay.


*****
Usage
*****

.. program-output:: pcsc-relay --help


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _`Hardware supported by libnfc`: http://nfc-tools.org/index.php?title=Devices_compatibility_matrix
.. _libnfc: http://www.libnfc.org/
.. _OpenPICC: http://www.openpcd.org/OpenPICC
.. _PCSC-lite: http://pcsclite.alioth.debian.org/
