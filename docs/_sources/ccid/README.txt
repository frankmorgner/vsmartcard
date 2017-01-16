.. highlight:: sh

.. |PACE| replace:: :abbr:`PACE (Password Authenticated Connection Establishment)`

.. _ccid-emulator:

################################################################################
USB CCID Emulator
################################################################################

.. sidebar:: Emulate a USB CCID compliant smart card reader

    :License:
        GPL version 3
    :Tested Platforms:
        Linux (Debian, Ubuntu, OpenMoko)

The USB CCID Emulator forwards a locally present PC/SC smart card reader as a
standard USB CCID reader. USB CCID Emulator can be used as trusted intermediary
enabling secure PIN entry and PIN modification. In combination with OpenSC_
also |PACE| can be performed by the emulator.

.. tikz:: Portable smart card reader with trusted user interface
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows

    \input{$wd/bilder/tikzstyles.tex}
    \tikzstyle{bla}=[kleiner, text width=.45\textwidth]

    \node (reader)
    {\includegraphics[width=1cm]{$wd/bilder/my_cardreader.pdf}};
    \node (readertext) [right=0of reader, bla]
    {Smartphone provides smart card reader via USB};
    \node (display) [below=0of reader]
    {\includegraphics[width=1cm]{$wd/bilder/display.pdf}};
    \node (displaytext) [right=0of display, bla]
    {Secure display of service provider and purpose of transaction};
    \node (keyboard) [below=0of display]
    {\includegraphics[width=1cm]{$wd/bilder/keyboard.pdf}};
    \node (keyboardtext) [right=0of keyboard, bla]
    {Secure PIN Entry};
    \node (firewall) [below=0of keyboard]
    {\includegraphics[width=1cm]{$wd/bilder/Firewall.pdf}};
    \node (firewalltext) [right=0of firewall, bla]
    {Verification of terminal authentication and sanitiy checks};

    \node (features) [fit=(display) (keyboard) (reader) (firewall)] {};

    \node (moko) [left=0of features.west] {\includegraphics[height=4cm]{$wd/bilder/phone-fic-neo-freerunner.pdf}};

    \node (epa) [left=1.5of moko, yshift=-2cm]
    {\includegraphics[width=3cm]{$wd/bilder/nPA_VS.png}};
    \node (pc)  [left=1.5of moko, yshift=1.5cm]
    {\includegraphics[width=3cm]{$wd/bilder/computer-tango.pdf}};

    \begin{pgfonlayer}{background}

        \node (mokobox) 
        [box,
        fit=(moko) (readertext) (displaytext) (keyboardtext) (firewalltext)
        (features)] {};

        \draw [usb]
        (moko) -- (pc.center) ;
        \draw [decorate, decoration={expanding waves, angle=20, segment length=6}, nichtrundelinie]
        (moko) -- (epa) ;

    \end{pgfonlayer}


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

    \input{$wd/bilder/tikzstyles.tex}
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

	\node (opensc) [box, below=of ccid, kleiner] {OpenSC};

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


Running the USB CCID Emulator has the following dependencies:

- Linux Kernel with GadgetFS_
- OpenSC_

Whereas using the USB CCID Emulator on the host system as smart card reader only
needs a usable PC/SC middleware with USB CCID driver. This is the case for most
modern Windows and Unix-like systems by default.

.. tikz:: Implementation of a mobile smart card reader for the German ID card
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows

    \input{$wd/bilder/tikzstyles.tex}
    \tikzstyle{keks}=[to path={-- ++(.1,0) |- (\tikztotarget)}]

    \tikzstyle{bla}=[shape=rectangle split, rectangle split parts=2,
    every text node part/.style={align=center, klein}, text width=7cm,
    every second node part/.style={kleiner}, inner sep=0pt]

    \node (ccid-emulator)
    {\texttt{ccid-emulator}};

    \node (basis) [below=3of ccid-emulator]
    {\includegraphics[keepaspectratio, height=2cm,
        width=2cm]{$wd/bilder/moko/basisleser_plain_klein.png}};
    \node (basisbeschreibung) [below=0cm of basis, kleiner, text width=2cm]
    {Reiner SCT RFID basis};

    \node (npa) [left=1.5of basis]
    {\includegraphics[keepaspectratio, height=3cm,
        width=3cm]{$wd/bilder/nPA_VS.png}};
    \node (npabeschreibung) [below=0cm of npa, kleiner]
    {German identity card};

    \node (funktionenchat) [right=.6cm of ccid-emulator.east, anchor=text west, bla]
    {
        PACE
        \nodepart{second}
        \begin{itemize}

        \item Display CHAT
            \begin{itemize}
                \item Display context (eID/eSign)
                \item Display requested permissions
            \end{itemize}

            \item Display certificate description
            \begin{itemize}
                \item Identification of service provider
                \item Display purpose of transaction
            \end{itemize}

            \item Secure PIN entry
        \end{itemize}
    };
    \node (funktionenpace) [below=.5 of funktionenchat, bla]
    {
        Terminal Authentication
        \nodepart{second}
		\begin{itemize}
            \item Verify authenticy of terminal
            \item Check freshness of cv certificate
        \end{itemize}
    };

    \begin{pgfonlayer}{background}
        \node (box) [fit=(ccid-emulator) (basis) (basisbeschreibung)
        (funktionenchat) (funktionenpace), box, inner sep=.5cm] {};
        \node (boxbild) at (box.north west)
        {\includegraphics[keepaspectratio, height=1.5cm,
        width=1.5cm]{$wd/bilder/moko/moko_reader.png}};
        \node [right=0cm of boxbild.east, yshift=.3cm]
        {Openmoko Neo FreeRunner};
    \end{pgfonlayer}

    \node (a) [above=1of npa]
	{\includegraphics[keepaspectratio, height=3cm,
	width=3cm]{$wd/bilder/computer-tango.pdf}};


    \begin{pgfonlayer}{background}
        \path
        (ccid-emulator) edge [doppelpfeil] (basis)
        (basis) edge [rfid] (npa)
        (a.center) edge [usb] (ccid-emulator)
        (ccid-emulator.east) edge [pfeil, keks] (funktionenchat.text west)
        (ccid-emulator.east) edge [pfeil, keks] (funktionenpace.text west);
    \end{pgfonlayer}


.. include:: download.txt


.. include:: autotools.txt


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

USB CCID Emulator needs the OpenSC components to be
installed (especially :file:`libopensc.so`). Here is an example of how to get
the standard installation of OpenSC without |PACE|::

    PREFIX=/tmp/install
    VSMARTCARD=$PWD/vsmartcard
    git clone https://github.com/frankmorgner/vsmartcard.git $VSMARTCARD
    cd $VSMARTCARD
    git submodule init
    git submodule update
    cd $VSMARTCARD/ccid/src/opensc
    autoreconf --verbose --install
    ./configure --prefix=$PREFIX
    make install && cd -

Now :file:`libopensc.so` should be located in ``$PREFIX/lib``. Here is how to
configure the USB CCID Emulator to use it::

    cd $VSMARTCARD/ccid
    ./configure --prefix=$PREFIX OPENSC_LIBS="-L$PREFIX/lib -lopensc"
    make install && cd -


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
.. _`OpenSC`: https://github.com/frankmorgner/OpenSC
.. _`libccid`: http://pcsclite.alioth.debian.org/ccid.html
.. _`Windows USB CCID driver`: http://msdn.microsoft.com/en-us/windows/hardware/gg487509
.. _`OpenMoko Wiki`: http://wiki.openmoko.org/wiki/Building_Gadget_USB_Module
.. _`BSI TR-03119 1.3`: https://www.bsi.bund.de/DE/Publikationen/TechnischeRichtlinien/tr03119/index_htm.html
