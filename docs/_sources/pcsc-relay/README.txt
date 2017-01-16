.. highlight:: sh

.. _pcsc-relay:

################################################################################
PC/SC Relay
################################################################################

.. sidebar:: Relay a smart card to a contactless interface

  :License:
      GPL version 3
  :Tested Platforms:
      - Windows
      - Linux (Debian, Ubuntu, OpenMoko)

Welcome to PC/SC Relay. The purpose of PC/SC Relay is to relay a smart
card using an contact-less interface. Currently the following contact-less
emulators are supported:

- `Hardware supported by libnfc`_
- OpenPICC_
- :ref:`acardemulator`

Command APDUs are received with the contact-less interface and relayed. The
Response APDUs are then sent back via RFID. The contact-less data will be
relayed to one of the following:

- to a *real* smart card inserted into one of the systems' smart card readers.
  The smart card reader must be accessible with PC/SC. The smart card may be
  contact-based *or* contact-less.
- to a :ref:`vicc` that directly connects to :command:`pcsc-relay`. The virtual
  smart card's native interface is used and (despite its name) PC/SC Relay
  does not need to access PC/SC in this case.

.. tikz:: Debug, Analyze and Emulate with PC/SC Relay
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows

    \input{$wd/bilder/tikzstyles.tex}
    \node (pcsc-relay) {\texttt{pcsc-relay}};

    \node [kleiner, right=2cm of pcsc-relay, yshift=.9cm]
    (libnfc) {Libnfc Devices};
    \node [kleiner, right=2cm of pcsc-relay, yshift=.3cm]
    (openpicc) {OpenPICC};
    \node [kleiner, right=2cm of pcsc-relay, yshift=-.3cm]
    (vpcd) {Virtual Smart Card Reader};
    \node [kleiner, right=2cm of pcsc-relay, yshift=-.9cm]
    (acardemulator) {Android Smart Card Emulator};

    \node [kleiner, left=2cm of pcsc-relay, yshift=.9cm]
    (contactbased) {Contact-based Smart Card};
    \node [kleiner, left=2cm of pcsc-relay, yshift=.3cm]
    (contactless) {Contact-less Smart Card};
    \node [kleiner, left=2cm of pcsc-relay, yshift=-.9cm]
    (remotereader) {Remote Smart Card Reader};
    \node [kleiner, left=2cm of pcsc-relay, yshift=-.3cm]
    (virtual) {Virtual Smart Card};

    \node [above=.3cm of pcsc-relay, kleiner, xshift=1.2cm]
    (capdu) {Command APDU};
    \node [below=.3cm of pcsc-relay, kleiner, xshift=-1.2cm]
    (rapdu) {Response APDU};
    \draw (rapdu.east) edge [pfeil] +(1,0);
    \draw (capdu.west) edge [pfeil] +(-1,0);

    \begin{pgfonlayer}{background}
    \path[line width=.5cm,color=hublue!20]
    (pcsc-relay.mid) edge [out=180, in=0] (contactbased.east)
    edge [out=180, in=0] (contactless.east)
    edge [out=180, in=0] (virtual.east)
    edge [out=180, in=0] (remotereader.east)
    edge [out=0, in=180] (libnfc.west)
    edge [out=0, in=180] (openpicc.west)
    edge [out=0, in=180] (vpcd.west)
    edge [out=0, in=180] (acardemulator.west)
    ;
    \end{pgfonlayer}


With PC/SC Relay you can relay a contact-less or contact based smart card
over a long distance. Also you can use it in combination with the :ref:`vicc`
to completely emulate an ISO/IEC 14443 smart card.

.. tikz:: Emulate a contact-less German ID card to perform sanity checks
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows

    \input{$wd/bilder/tikzstyles.tex}
    \tikzstyle{bla}=[shape=rectangle split, rectangle split parts=2,
    every text node part/.style={align=center, klein}, text width=7cm,
    every second node part/.style={kleiner}, inner sep=0pt]

    \tikzstyle{keks}=[to path={-- ++(.1,0) |- (\tikztotarget)}]

    \node (touchatag)
	{\includegraphics[keepaspectratio, height=2cm,
	width=2cm]{$wd/bilder/touchatag.png}};
    \node (touchatagbeschreibung) [below=0cm of touchatag, kleiner]
    {touchatag};

    \node (pcsc-relay) [right=of touchatag]
    {\texttt{pcsc-relay}};
    \node (pcsc-relaybeschreibung) [below=0cm of pcsc-relay, kleiner]
    {NFC emulator
    };

    \node (vicc) [right=of pcsc-relay]
    {\texttt{vicc -t nPA}};
    \node (viccbeschreibung) [below=0cm of vicc, kleiner]
    {nPA emulator};

    \node (group) [fit=(touchatag) (pcsc-relay) (vicc) (touchatagbeschreibung)
    (pcsc-relaybeschreibung) (viccbeschreibung)] {};
    \node (funktionenchat) [below=0cm of group, bla]
    {
        PACE
        \nodepart{second}
		\begin{itemize}
            \item Display context (eID/eSign)
            \item Display requested permissions
        \end{itemize}
    };
    \node (funktionenpace) [below=.5 of funktionenchat, bla]
    {
        Terminal Authentication
        \nodepart{second}
		\begin{itemize}
            \item Verify authenticy of terminal
            \item Check freshness of cv certificate
            \item With certificate database
                \begin{itemize}
                    \item Identification of service provider
                    \item Display purpose of transaction
                \end{itemize}
        \end{itemize}
    };

    \begin{pgfonlayer}{background}
        \node (box) [fit=(touchatag) (pcsc-relay) (vicc) (touchatagbeschreibung)
        (pcsc-relaybeschreibung) (viccbeschreibung) (funktionenchat)
        (funktionenpace), box, inner sep=.5cm] {};
        \node (boxbild) at (box.north west)
        {\includegraphics[keepaspectratio, height=1.5cm,
        width=1.5cm]{$wd/bilder/moko/moko_reader.png}};
        \node [right=0cm of boxbild.east, yshift=.3cm]
        {Openmoko Neo FreeRunner};
    \end{pgfonlayer}

    \node (a) [left=1.5of box]
	{\includegraphics[keepaspectratio, height=4cm,
	width=4cm]{$wd/bilder/ivak_kiosk-terminal.pdf}};
    \node (e) [below=0cm of a, text width=2.5cm, align=center]
    {(Public) Terminal};


    \begin{pgfonlayer}{background}
        \path
        (touchatag)  edge  [doppelpfeil] (pcsc-relay)
        (pcsc-relay) edge [doppelpfeil] (vicc)
        (vicc.east) edge [keks, pfeil] (funktionenchat.text east)
        (vicc.east) edge [keks, pfeil] (funktionenpace.text east)
        (a) edge [decorate, decoration={expanding waves, angle=20, segment
        length=6}, nichtrundelinie] (touchatag.south west);
    \end{pgfonlayer}


PC/SC Relay has the following dependencies:

- PC/SC middleware
- libnfc_


.. include:: relay-note.txt


.. include:: download.txt


.. include:: autotools.txt


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


====================================
Hints on Android Smart Card Emulator
====================================

The Android Smart Card Emulator is build around the host card emulation mode of
Android 4.4 and later. This mode activates the app if the terminal issues a
SELECT command with one of the app's application identifiers. By default, the
app only registers for the AIDs for which it has a built-in emulator (see
:file:`ACardEmulator/app/src/main/res/xml/aid_list.xml`).

If used together with PC/SC Relay, you need to change add AIDs to match the
applications on the relayed card.  Otherwise the app will not be activated when
it should relay command APDUs to PC/SC Relay.

Modify the Smart Card Emulator settings to use ``Remote Virtual Smart Card`` as
:guilabel:`Smart Card Emulator`. Now start :command:`pcsc-relay` by specifying
usage of the vpcd emulator::

    pcsc-relay --emulator vpcd

In the app, change the :guilabel:`VICC Hostname` and :guilabel:`VICC Port` to
match the location where :command:`pcsc-relay` is waiting for an incoming
connection. When the app receives a SELECT command to one of the configured
AIDs, it will connect to :command:`pcsc-relay`, which will then relay the
command for processing.

Compiling and installing Android Smart Card Emulator is covered in its
:ref:`acardemulator_install` section.


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

Below we explain what option to choose for the emulator which receives a
command APDU and transmits a response APDU back to the terminal:

=================================================== ==============
Option                                              ``--emulator``
=================================================== ==============
Emulation hardware supported via libnfc             ``libnfc``
Emulation with OpenPICC                             ``openpicc``
Android Smart Card Emulator                         ``vpcd``
Virtual Smart Card                                  ``vpcd``
=================================================== ==============

Below we explain what option to choose for the connector which calculates
a response APDU from a given command APDU:

=================================================== ===============
Option                                              ``--connector``
=================================================== ===============
Contact-based Smart Card in PC/SC Reader            ``pcsc``
Contact-less Smart Card in PC/SC Reader             ``pcsc``
Contact-less Smart Card in Remote Smart Card Reader ``vicc``
Virtual Smart Card                                  ``vicc``
=================================================== ===============


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _`Hardware supported by libnfc`: http://nfc-tools.org/index.php?title=Devices_compatibility_matrix
.. _libnfc: http://www.libnfc.org/
.. _OpenPICC: http://www.openpcd.org/OpenPICC
.. _PCSC-lite: http://pcsclite.alioth.debian.org/
