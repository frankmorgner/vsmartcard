.. highlight:: sh

.. |vpcd| replace:: :abbr:`vpcd (virtual smart card reader)`

.. _remote-reader:

################################################################################
Remote Smart Card Reader
################################################################################

.. sidebar:: Summary

    Use an Android smartphone with NFC as contact-less smart card reader

    :Author:
        `Frank Morgner <morgner@informatik.hu-berlin.de>`_
    :License:
        GPL version 3
    :Tested Platform:
        Android, CyanogenMod

The Remote Smart Card Reader allows a host computer to use the smartphone's NFC
hardware as contact-less smart card reader. On the host computer, a special
smart card driver, |vpcd| must be installed. The Remote Smart Card Reader
establishes a connection to |vpcd| over the network when a contact-less card is
detected. Since |vpcd| integrates seamlessly into the PC/SC framework of the
host system, so the contact-less card can be accessed by a typical smart card
application, for example OpenSC_.

.. tikz:: Remote Smart Card Reader used to access a contact-less card
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{%(wd)s/bilder/tikzstyles.tex}
	\node (pcsc) [klein, aktivbox, inner xsep=.75cm, align=center] {PC/SC\\Framework};
    \node (sca) [aktivbox, klein, left=.75cm of pcsc, align=center] {Smart Card\\Application};
    \node (vpcd) [box, at=(pcsc.east), kleiner] {\texttt{vpcd}};
    \node (phone) [right=1cm of vpcd] {\includegraphics[width=3cm]{%(wd)s/bilder/smartphone.pdf}};
    \node (app) [at=(phone.center)] {\includegraphics[width=2.8cm, height=4.9cm]{%(wd)s/bilder/remote-reader.png}};
    \node (card) [right=0cm of phone, rotate=45] {\includegraphics[width=2cm]{%(wd)s/bilder/smartcard.pdf}};

    \begin{pgfonlayer}{background}
        \node (box) [box, fit=(pcsc) (sca) (vpcd), inner ysep=1.5em] {};
        \node [at=(box.north west)] {\includegraphics[width=1cm]{%(wd)s/bilder/computer-tango.pdf}};
        \path[linie]
        (sca) edge (pcsc)
        (vpcd) edge (app)
        ;
        \draw [rfid] (phone.center) -- (card.center) ;
    \end{pgfonlayer}

The Remote Smart Card Reader has the following dependencies:

- NFC hardware built into the smartphone
- Android 3.0 "Honeycomb" (or newer) or CyanogenMod 8 (or newer)
- permissions for a data connection (communication with |vpcd|) and for using
  NFC (communication to the card)
- |vpcd| :ref:`installed on the host computer<vicc_install>`

For remotely accessing a traditional smart card reader on one computer from an
other computer, the :ref:`vicc` in relay mode can be used:

.. tikz:: Virtual Smart Card used in relay mode to remotely access a card
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{%(wd)s/bilder/tikzstyles.tex}
	\node (pcsc) [klein, aktivbox, inner xsep=.75cm, align=center] {PC/SC\\Framework};
    \node (sca) [aktivbox, klein, left=.75cm of pcsc, align=center] {Smart Card\\Application};
    \node (vpcd) [box, at=(pcsc.east), kleiner] {\texttt{vpcd}};
	\node (pcsc2) [klein, aktivbox, inner xsep=.75cm, align=center, below=2.5cm of vpcd] {PC/SC\\Framework};
    \node (vicc) [aktivbox, left=1cm of pcsc2, kleiner] {\texttt{vicc -t relay}};
    \node (reader) [right=1cm of pcsc2] {\includegraphics[width=2cm]{%(wd)s/bilder/my_cardreader.pdf}};
    \node (card) [at=(reader.east), rotate=45] {\includegraphics[width=2cm]{%(wd)s/bilder/smartcard.pdf}};

    \begin{pgfonlayer}{background}
        \node (box) [box, fit=(pcsc) (sca) (vpcd), inner ysep=1.5em] {};
        \node (box2) [box, fit=(vicc) (pcsc2), inner ysep=1.5em] {};
        \node [at=(box.north west)] {\includegraphics[width=1cm]{%(wd)s/bilder/computer-tango.pdf}};
        \node [at=(box2.north west)] {\includegraphics[width=1cm]{%(wd)s/bilder/computer-tango.pdf}};
        \path[linie]
        (sca) edge (pcsc)
        (vpcd) edge [out=south, in=north] (vicc)
        (vicc) edge (pcsc2)
        (pcsc2) edge (reader)
        ;
    \end{pgfonlayer}



.. include:: relay-note.txt


********************
Download and Install
********************

The Remote Smart Card Reader is available on F-Droid_.

.. qr code generated via http://www.qrcode-monkey.de

.. image:: remote-reader-qrcode.png
    :target: https://f-droid.org/repository/browse/?fdid=com.vsmartcard.remotesmartcardreader.app  
    :alt: Remote Smart Card Reader on F-Droid
    :width: 265px
    :height: 265px

To manually compile the app you need to fetch the sources::

    git clone https://github.com/frankmorgner/vsmartcard.git

We use `Android Studio`_ to build and deploy the application. Use
:menuselection:`File --> Open` to select :file:`vsmartcard/remote-reader`.
Attach your smartphone and choose :menuselection:`Run --> Run 'app'`.


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _OpenSC: https://github.com/OpenSC/OpenSC
.. _Android Studio: http://developer.android.com/sdk/installing/studio.html
.. _F-Droid: https://f-droid.org/repository/browse/?fdid=com.vsmartcard.remotesmartcardreader.app
