.. highlight:: sh

.. |libnpa| replace:: :ref:`libnpa`
.. |PACE| replace:: :abbr:`PACE (Password Authenticated Connection Establishment)`

.. _ccid-emulator:

################################################################################
USB CCID Emulator
################################################################################

.. sidebar:: Summary

    Emulate a USB CCID compliant smart card reader

    :Author:
        `Frank Morgner <morgner@informatik.hu-berlin.de>`_
    :License:
        GPL version 3
    :Tested Platforms:
        Linux (Debian, Ubuntu, OpenMoko)

The USB CCID Emulator forwards a locally present PC/SC smart card reader as a
standard USB CCID reader. USB CCID Emulator can be used as trusted intermediary
enabling secure PIN entry and PIN modification. In combination with the |libnpa|
also |PACE| can be performed by the emulator.

If the machine running :command:`ccid-emulator` is in USB device mode, a local
reader is forwareded via USB to another machine. If in USB host mode, the USB
CCID reader will locally be present.

Applications on Windows and Unix-like systems can access the USB CCID Emulator
through PC/SC as if it was a real smart card reader. No installation of a smart
card driver is required since USB CCID drivers are usually shipped with the
modern OS.

Here is a subset of USB CCID commands supported by the USB CCID Emulator with
their PC/SC counterpart:

================================== ============================================================
USB CCID                           PC/SC
================================== ============================================================
``PC_to_RDR_XfrBlock``             ``SCardTransmit``
``PC_to_RDR_Secure``               ``FEATURE_VERIFY_PIN_DIRECT``, ``FEATURE_MODIFY_PIN_DIRECT``
``PC_to_RDR_Secure`` (proprietary) ``FEATURE_EXECUTE_PACE``
================================== ============================================================

PIN verification/modification and |PACE| can also be started by the application
transmitting (SCardTransmit) specially crafted APDUs.  Only the alternative
initialization of |PACE| using SCardControl requires patching the driver
(available for libccid, see :file:`patches`). The pseudo APDUs with no need for
patches are defined as follows (see `BSI TR-03119 1.3`_ p. 33-34):

+--------------------------+----------------------------------------------------------------------------+------------------------------------------------+
|                          | Command APDU                                                               | Response APDU                                  |
|                          +----------+----------+----------+----------+--------------------------------+-----------------------------------+------------+
|                          | CLA      | INS      | P1       | P2       | Command Data                   | Response Data                     | SW1/SW2    |
+==========================+==========+==========+==========+==========+================================+===================================+============+
| GetReaderPACECapabilities| ``0xFF`` | ``0x9A`` | ``0x04`` | ``0x01`` | (No Data)                      | ``PACECapabilities``              |            |
+--------------------------+----------+----------+----------+----------+--------------------------------+-----------------------------------+ ``0x9000`` |
| EstablishPACEChannel     | ``0xFF`` | ``0x9A`` | ``0x04`` | ``0x02`` | ``EstablishPACEChannelInput``  | ``EstablishPACEChannelOutput``    | or other   |
+--------------------------+----------+----------+----------+----------+--------------------------------+-----------------------------------+ in case of |
| DestroyPACEChannel       | ``0xFF`` | ``0x9A`` | ``0x04`` | ``0x03`` | (No Data)                      | (No Data)                         | an error   |
+--------------------------+----------+----------+----------+----------+--------------------------------+-----------------------------------+            |
| Verify/Modify PIN        | ``0xFF`` | ``0x9A`` | ``0x04`` | ``0x10`` | Coding as ``PC_to_RDR_Secure`` | Coding as ``RDR_to_PC_DataBlock`` |            |
+--------------------------+----------+----------+----------+----------+--------------------------------+-----------------------------------+------------+

The USB CCID Emulator is implemented using GadgetFS_. Some fragments of the source
code are based on the GadgetFS example and on the source code of the OpenSC
tools.

.. tikz:: Software stack of the USB CCID Emulator running on the OpenMoko Neo FreeRunner
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows

    \input{%(wd)s/bilder/tikzstyles.tex}
    \tikzstyle{schicht}=[text width=5cm, align=right]
    \tikzstyle{fade down}=[path fading=south, color=huslateblue]

	\node (kernel)
    [box, shape=rectangle split, rectangle split parts=3, kleiner]
	{Linux  Kernel
	\nodepart{second}
    \footnotesize S3C24xx Controller Driver
	\nodepart{third}
    \footnotesize GadgetFS
	};

	\node (ccid)
	[aktivbox, shape=rectangle split, rectangle split parts=2, below=of kernel]
	{CCID Emulator
	\nodepart{second}
    \texttt{usb}\qquad\texttt{ccid}
	};
	\draw [box] ($(ccid.text split)-(.05cm,0)$) -- ($(ccid.south)-(.05cm,0)$);

	\node (opensc) [box, below=of ccid, kleiner] {\texttt{libnpa} or OpenSC};

    \node (controller) [klein, right=0of kernel.two east, schicht]
    {Driver for USB Controller};
    \node (gadget) [klein, right=0of kernel.three east, schicht]
    {Gadget Driver};
    \node (upper) [klein, right=0of kernel.three east, schicht, yshift=-1.75cm]
    {Upper Layers};

    \tikzstyle{rechts}=[to path={-- ++(1,0) |- (\tikztotarget)}]
    \tikzstyle{links}=[to path={-- ++(-1,0) |- (\tikztotarget)}]
    \begin{pgfonlayer}{background}
        \path
        (ccid.two west) edge [links, linie] (kernel.three west)
        (ccid.two east) edge [rechts, linie] (opensc.east)
        ;

        \path [color=black!30]
        (controller.north east) edge +(-9,0)
        (gadget.north east) edge +(-9,0)
        (upper.north east) edge +(-9,0)
        ;
    \end{pgfonlayer}

    \node [kleiner, anchor=east, text width=3cm]
    at ($($(ccid.two west)+(-3,0)$)!.5!(kernel.three west)$)
    {\color{hublue}
    \texttt{/dev/gadget/ep1-bulk}\\
    \texttt{/dev/gadget/ep2-bulk}\\
    \texttt{/dev/gadget/ep3-bulk}\\};



.. include:: download.txt


.. include:: autotools.txt

Running the USB CCID Emulator has the following dependencies:

- Linux Kernel with GadgetFS_
- OpenSC_
- |libnpa| (only if support for |PACE| is enabled)

Whereas using the USB CCID Emulator on the host system as smart card reader only
needs a usable PC/SC middleware with USB CCID driver. This is the case for most
modern Windows and Unix-like systems by default.


=================
Hints on GadgetFS
=================

To create a USB Gadget in both USB host and USB client mode, you need to load
the kernel module :program:`gadgetfs`. Here is how to get a running version of
GadgetFS on a Debian system (see also `OpenMoko Wiki`_)::

    sudo apt-get install linux-source linux-headers-`uname -r`
    sudo tar xjf /usr/src/linux-source-*.tar.bz2
    cd linux-source-*/drivers/usb/gadget
    # build dummy_hcd and gadgetfs
    echo "KDIR := /lib/modules/`uname -r`/build" >> Makefile
    echo "PWD := `pwd`" >> Makefile
    echo "obj-m := dummy_hcd.o gadgetfs.o" >> Makefile
    echo "default: " >> Makefile
    echo -e "\t\$(MAKE) -C \$(KDIR) SUBDIRS=\$(PWD) modules" >> Makefile
    make
    # load GadgetFS with its dependencies
    sudo modprobe udc-core
    sudo insmod ./dummy_hcd.ko
    sudo insmod ./gadgetfs.ko default_uid=`id -u`
    # mount GadgetFS
    sudo mkdir /dev/gadget
    sudo mount -t gadgetfs gadgetfs /dev/gadget

On OpenMoko it is likely that you need to `patch your kernel
<http://docs.openmoko.org/trac/ticket/2206>`_. If you also want to switch
multiple times between :program:`gadgetfs` and :program:`g_ether`, `another
patch is needed <http://docs.openmoko.org/trac/ticket/2240)>`_.

If you are using a more recent version of :program:`dummy_hcd` and get an error
loading the module, you maybe want to check out `this patch
<http://comments.gmane.org/gmane.linux.usb.general/47440>`_.


===============
Hints on OpenSC
===============

Without the |libnpa| the USB CCID Emulator needs the OpenSC components to be
installed (especially :file:`libopensc.so`). Here is an example of how to get
the standard installation of OpenSC without |PACE|::

    PREFIX=/tmp/install
    VSMARTCARD=$PWD/vsmartcard
    git clone https://github.com/frankmorgner/vsmartcard.git $VSMARTCARD
    cd $VSMARTCARD/ccid/src/opensc
    autoreconf --verbose --install
    ./configure --prefix=$PREFIX
    make install && cd -

Now :file:`libopensc.so` should be located in ``$PREFIX/lib``. Here is how to
configure the USB CCID Emulator to use it::

    cd $VSMARTCARD/ccid
    ./configure --prefix=$PREFIX OPENSC_LIBS="-L$PREFIX/lib -lopensc"
    make install && cd -

If you want |PACE| support for the emulated smart card reader the process is
similar. Install |libnpa| and it should be recognized automatically by the
:file:`configure` script.


*****
Usage
*****

The USB CCID Emulator has various command line options to customize the appearance
on the USB host. In order to run the USB CCID Emulator GadgetFS must be loaded
and mounted.  The USB CCID Emulator is compatible with the unix driver libccid_
and the `Windows USB CCID driver`_. PIN commands are supported if implemented
by the driver.

.. versionadded:: 0.7
    USB CCID Emulator now supports the boxing commands defined in `BSI TR-03119
    1.3`_.


.. program-output:: ccid-emulator --help


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _`GadgetFS`: http://www.linux-usb.org/gadget/
.. _`OpenSC`: http://www.opensc-project.org/opensc
.. _`libccid`: http://pcsclite.alioth.debian.org/ccid.html
.. _`Windows USB CCID driver`: http://msdn.microsoft.com/en-us/windows/hardware/gg487509
.. _`OpenMoko Wiki`: http://wiki.openmoko.org/wiki/Building_Gadget_USB_Module
.. _`BSI TR-03119 1.3`: https://www.bsi.bund.de/DE/Publikationen/TechnischeRichtlinien/tr03119/index_htm.html
