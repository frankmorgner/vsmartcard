.. highlight:: sh

.. _OpenSC: http://www.opensc-project.org/opensc
.. _GadgetFS: http://www.linux-usb.org/gadget/
.. _libccid: http://pcsclite.alioth.debian.org/ccid.html

.. |npa| replace:: :ref:`npa`


********************************************************************************
USB CCID Emulator
********************************************************************************

:Author:
    Frank Morgner <morgner@informatik.hu-berlin.de>
:License:
    GPL version 3
:Tested Platforms:
    Linux (Debian, Ubuntu, OpenMoko)

Welcome to the USB CCID Emulator.  The purpose of the USB CCID Emulator is to forward
a PC/SC smartcard reader as a standard USB CCID reader. If the machine running
the USB CCID Emulator is in USB device mode, a local reader is forwareded via USB
to another machine. If in USB host mode, a USB CCID reader is virtually plugged
into the machine running the USB CCID Emulator. Applications on Windows and
Unix-like systems can access the USB CCID Emulator through PC/SC as if it were a
real smart card reader.

The USB CCID Emulator accesses a smart card through a local reader. Simple
commands such as transmitting an APDU (``SCardTransmit`` and accordingly
``PC_to_RDR_XfrBlock``) are directly forwarded to the local reader/smart card.
USB CCID Emulator can perform secure PIN verification and modification
(``FEATURE_VERIFY_PIN_DIRECT`` or ``FEATURE_MODIFY_PIN_DIRECT`` and accordingly
``PC_to_RDR_Secure``). Moreover the USB CCID Emulator has support the for Password
Authenticated Connection Establishment (PACE) using |npa|
(``FEATURE_EXECUTE_PACE``). Thus USB CCID Emulator can be used with the German
identity card ("neuer Personalausweis", nPA) similar to a "Standardleser"
(CAT-S) or "Komfortleser" (CAT-K).

The USB CCID Emulator is implemented using GadgetFS_. Some fragments of the source
code are based on the GadgetFS example and on the source code of the OpenSC_
tools.


.. include:: autotools.rst

Running the USB CCID Emulator has the following dependencies:

- Linux Kernel with GadgetFS_
- OpenSC_
- |npa| (only if support for PACE is enabled)

Whereas using the USB CCID Emulator on the host system as smart card reader only
needs a usable PC/SC middleware with USB CCID driver. This is the case for most
modern Windows and Unix-like systems by default.


-----------------
Hints on GadgetFS
-----------------

To create a USB Gadget in both USB host and USB client mode, you need to load
the kernel module :program:`gadgetfs`. A guide focused on Debian based systems to run
and compile :program:`gadgetfs`, you can find `here
<http://wiki.openmoko.org/wiki/Building_Gadget_USB_Module>`_.

On OpenMoko it is likely that you need to `patch your kernel
<http://docs.openmoko.org/trac/ticket/2206>`_. If you also want to switch
multiple times between :program:`gadgetfs` and :program:`g_ether`, `another patch is needed
<http://docs.openmoko.org/trac/ticket/2240)>`_.

If you are using a more recent version of :program:`dummy_hcd` and get an error
loading the module, you maybe want to check out `this patch
<http://comments.gmane.org/gmane.linux.usb.general/47440>`_.


---------------
Hints on OpenSC
---------------

Without the |npa| the USB CCID Emulator links against libopensc, which is
discouraged and hindered since OpenSC version >= 0.12. (We really need to get
rid of this dependency or integrate better into the OpenSC-framework.) You need
the OpenSC components to be installed (especially :file:`libopensc.so`). Here
is an example of how to get the standard installation of OpenSC_::

    PREFIX=/tmp/install
    OPENSC=opensc
    svn co http://www.opensc-project.org/svn/opensc/trunk $OPENSC
    cd $OPENSC
    autoreconf -i
    ./configure --prefix=$PREFIX
    make
    make install

Now :file:`libopensc.so` should be located in ``$PREFIX/lib``. Here is how to
configure the USB CCID Emulator to use it::

    ./configure OPENSC_LIBS="-L$PREFIX/lib -lopensc"


=====
Usage
=====

The USB CCID Emulator has various command line options to customize the appearance on
the USB host. In order to run the USB CCID Emulator GadgetFS_ must be loaded and
mounted.  The USB CCID Emulator is compatible with the unix driver libccid_ and the
windows smart card driver. To initialize PACE using the PC/SC API you need to
patch libccid and pcsc-lite (see directory patches).

.. program-output:: ccid-emulator --help

cats-test can be used to test the PACE capabilities of a smart card reader with
PACE support (such as the USB CCID Emulator or any other "Standardleser" CAT-S or
"Komfortleser" CAT-C) via PC/SC.

.. program-output:: cats-test


.. include:: questions.rst
