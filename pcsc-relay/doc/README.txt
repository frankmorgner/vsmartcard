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

- to a *real* smart card inserted into one of the systems' smart card readers.
  The smart card reader must be accessible with PC/SC. The smart card may be
  contact-based *or* contact-less.
- to a :ref:`vicc` that directly connects to :command:`pcsc-relay`. The virtual
  smart card's native interface is used and (despite its name) PC/SC Relay
  does not need to access PC/SC in this case.

With PC/SC Relay you can relay a contact-less or contact based smart card
over a long distance. Also you can use it in combination with the :ref:`vicc`
to completely emulate an ISO/IEC 14443 smart card.

.. tikz:: Emulate a contact-less German ID card to perform sanity checks
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows

    \input{%(wd)s/bilder/tikzstyles.tex}
    \tikzstyle{bla}=[shape=rectangle split, rectangle split parts=2,
    every text node part/.style={align=center, klein}, text width=7cm,
    every second node part/.style={kleiner}, inner sep=0pt]

    \tikzstyle{keks}=[to path={-- ++(.1,0) |- (\tikztotarget)}]

    \node (touchatag)
	{\includegraphics[keepaspectratio, height=2cm,
	width=2cm]{%(wd)s/bilder/touchatag.png}};
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
        width=1.5cm]{%(wd)s/bilder/moko/moko_reader.png}};
        \node [right=0cm of boxbild.east, yshift=.3cm]
        {Openmoko Neo FreeRunner};
    \end{pgfonlayer}

    \node (a) [left=1.5of box]
	{\includegraphics[keepaspectratio, height=4cm,
	width=4cm]{%(wd)s/bilder/ivak_kiosk-terminal.pdf}};
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
