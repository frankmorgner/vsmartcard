.. highlight:: sh

.. _OpenSC: http://www.opensc-project.org/opensc
.. _GadgetFS: http://www.linux-usb.org/gadget/
.. _libccid: http://pcsclite.alioth.debian.org/ccid.html


*************
ccid-emulator
*************

:Author:
    Frank Morgner <morgner@informatik.hu-berlin.de>
:License:
    GPL version 3
:Tested Platforms:
    Linux (Debian, Ubuntu, OpenMoko)

Welcome to ccid-emulator.  The purpose of ccid-emulator is to forward a PCSC
smartcard reader as a standard USB CCID reader. If the host system is in USB
device mode, ccid-emulator forwards the local reader via USB to another
device. If in USB host mode, ccid-emulator virtually plugges in a USB CCID
reader to the host system.  ccid-emulator has support for Password
Authenticated Connection Establishment (PACE) using OpenPACE
(http://sourceforge.net/projects/openpace/).

ccid-emulator is implemented using GadgetFS_. Some fragments of the source code
are based on the GadgetFS example and on the source code of the OpenSC_ tools.


.. include:: autotools.rst

ccid as the following dependencies:

- Linux Kernel with GadgetFS_
- OpenSC_
- :ref:`npa` (only if support for PACE is enabled)


-----------------
Hints on GadgetFS
-----------------

To create an USB Gadget in both USB host and USB client mode, you need to load
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

Without libnpa ccid-emulator links against libopensc, which is discouraged and hindered since OpenSC
version >= 0.12. (We really need to get rid of this dependency or integrate
better into the OpenSC-framework.) You need the OpenSC components to be
installed (especially :file:`libopensc.so`). Here is an example of how to get the
standard installation of OpenSC_::

    PREFIX=/tmp/install
    OPENSC=opensc
    svn co http://www.opensc-project.org/svn/opensc/trunk $OPENSC
    cd $OPENSC
    autoreconf -i
    ./configure --prefix=$PREFIX
    make
    make install

Now :file:`libopensc.so` should be located in ``$PREFIX/lib``. Here is how to
configure ccid-emulator to use it::

    ./configure OPENSC_LIBS="-L$PREFIX/lib -lopensc"


=====
Usage
=====

ccid-emulator has various command line options to customize the appearance on
the USB host. In order to run ccid-emulator GadgetFS_ must be loaded and
mounted.  ccid-emulator is compatible with the unix driver libccid_ and the
windows smart card driver. To initialize PACE using the PC/SC API you need to
patch libccid and pcsc-lite (see directory patches).

cats-test can be used to test the PACE capabilities of a smart card reader with
PACE support (such as ccid-emulator or any other "Standardleser" CAT-S or
"Komfortleser" CAT-C) via PC/SC.


.. include:: questions.rst
