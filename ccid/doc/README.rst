.. highlight:: sh

.. _OpenSC: http://www.opensc-project.org/opensc
.. _GadgetFS: http://www.linux-usb.org/gadget/
.. _libccid: http://pcsclite.alioth.debian.org/ccid.html

.. |npa| replace:: :ref:`npa`
.. |PACE| replace:: :abbr:`PACE (Password Authenticated Connection Establishment)`

.. _ccid-emulator:

********************************************************************************
USB CCID Emulator
********************************************************************************

:Author:
    Frank Morgner <morgner@informatik.hu-berlin.de>
:License:
    GPL version 3
:Tested Platforms:
    Linux (Debian, Ubuntu, OpenMoko)

The USB CCID Emulator forwards a locally present PC/SC smart card reader as a
standard USB CCID reader. USB CCID Emulator can be used as trusted intermediary
enabling secure PIN entry and PIN modification. In combination with the |npa|
also |PACE| can be performed by the emulator.

If the machine running :command:`ccid-emulator` is in USB device mode, a local
reader is forwareded via USB to another machine. If in USB host mode, the USB
CCID reader will locally be present.

Applications on Windows and Unix-like systems can access the USB CCID Emulator
through PC/SC as if it was a real smart card reader. No installation of a smart
card driver is required since USB CCID drivers are usually shipped with the
modern OS. [#f1]_

Here is a subset of USB CCID commands supported by the USB CCID Emulator with
their PC/SC counterpart:

================================== ============================================================
USB CCID                           PC/SC
================================== ============================================================
``PC_to_RDR_XfrBlock``             ``SCardTransmit``
``PC_to_RDR_Secure``               ``FEATURE_VERIFY_PIN_DIRECT``, ``FEATURE_MODIFY_PIN_DIRECT``
``PC_to_RDR_Secure`` (proprietary) ``FEATURE_EXECUTE_PACE``
================================== ============================================================

The USB CCID Emulator is implemented using GadgetFS_. Some fragments of the source
code are based on the GadgetFS example and on the source code of the OpenSC_
tools.


.. [#f1] Note that the heavily outdated `Windows USB CCID driver <http://msdn.microsoft.com/en-us/windows/hardware/gg487509>`_ does not support secure PIN entry or PIN modification. USB CCID Emulator comes with a patch for libccid_ to support |PACE|, because it is not yet standardised in USB CCID. However, the traditional commands can be used without restriction.


.. include:: autotools.rst

Running the USB CCID Emulator has the following dependencies:

- Linux Kernel with GadgetFS_
- OpenSC_
- |npa| (only if support for |PACE| is enabled)

Whereas using the USB CCID Emulator on the host system as smart card reader only
needs a usable PC/SC middleware with USB CCID driver. This is the case for most
modern Windows and Unix-like systems by default.


-----------------
Hints on GadgetFS
-----------------

To create a USB Gadget in both USB host and USB client mode, you need to load
the kernel module :program:`gadgetfs`. A guide focused on Debian based systems
to run and compile GadgetFS_, you can find `here
<http://wiki.openmoko.org/wiki/Building_Gadget_USB_Module>`_.

On OpenMoko it is likely that you need to `patch your kernel
<http://docs.openmoko.org/trac/ticket/2206>`_. If you also want to switch
multiple times between :program:`gadgetfs` and :program:`g_ether`, `another
patch is needed <http://docs.openmoko.org/trac/ticket/2240)>`_.

If you are using a more recent version of :program:`dummy_hcd` and get an error
loading the module, you maybe want to check out `this patch
<http://comments.gmane.org/gmane.linux.usb.general/47440>`_.


---------------
Hints on OpenSC
---------------

Without the |npa| the USB CCID Emulator links against OpenSC, which is discouraged
and hindered since OpenSC version >= 0.12. You need the OpenSC components to be
installed (especially :file:`libopensc.so`). Here is an example of how to get
the standard installation of OpenSC_ without |PACE|::

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

The USB CCID Emulator has various command line options to customize the appearance
on the USB host. In order to run the USB CCID Emulator GadgetFS_ must be loaded
and mounted.  The USB CCID Emulator is compatible with the unix driver libccid_
and the `Windows USB CCID driver
<http://msdn.microsoft.com/en-us/windows/hardware/gg487509>`_. To initialize
|PACE| using the PC/SC API you need to patch libccid_ (see :file:`patches`).

.. program-output:: ccid-emulator --help


.. include:: questions.rst
