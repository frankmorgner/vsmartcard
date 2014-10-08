.. highlight:: sh

.. |vpcd| replace:: :abbr:`vpcd (virtual smart card reader)`

.. _remote-reader:

################################################################################
Remote Smart Card Reader
################################################################################

.. sidebar:: Summary

    Use an Android phone as contact-less smart card reader

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
host system, the contact-less card can be accessed by a typical smart card
application, for example OpenSC_.

.. tikz:: Remote Smart Card Reader used to access a contact-less card
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{%(wd)s/bilder/tikzstyles.tex}
	\node (pcsc) [klein, aktivbox, inner xsep=.75cm, align=center] {PC/SC\\Framework};
    \node (sca) [aktivbox, klein, left=.75cm of pcsc, align=center] {Smart Card\\Application};
    \node (vpcd) [box, at=(pcsc.east), kleiner] {\texttt{vpcd}};
    \node (phone) [right=2cm of vpcd] {\includegraphics[width=3cm]{%(wd)s/bilder/smartphone.pdf}};
    \node (app) [at=(phone.center)] {\includegraphics[width=2.8cm, height=4.9cm]{%(wd)s/bilder/remote-reader.png}};
    \node (card) [right=0cm of phone, rotate=45] {\includegraphics[width=2cm]{%(wd)s/bilder/smartcard.pdf}};

    \begin{pgfonlayer}{background}
        \node (box) [box, fit=(pcsc) (sca) (vpcd), inner ysep=1.5em] {};
        \node [at=(box.north west)] {\includegraphics[width=1cm]{%(wd)s/bilder/computer-tango.pdf}};
        \path[linie]
        (sca) edge (pcsc)
        (vpcd) edge node {\includegraphics[width=1.5cm]{%(wd)s/bilder/simplecloud.pdf}} (app)
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
        (vpcd) edge [out=south, in=north] node {\includegraphics[width=1.5cm]{%(wd)s/bilder/simplecloud.pdf}} (vicc)
        (vicc) edge (pcsc2)
        (pcsc2) edge [usb] (reader.center)
        ;
    \end{pgfonlayer}



.. include:: relay-note.txt


.. note::
Without further changes, successful communication with German ID card is not possible.
Due to a limitation in Android NFC service itself, support of extended length APDUs is not possible at all.
Only on rooted devices with right NFC chip, the NFC service might be patched.
For details please refer to `Android Open Source Project - Issue Tracker`_
A second limitation is that only some NFC chip manufacturer supports extended length APDUs.

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

On the host system, where the smart card at the phone's NFC interface is relayed to,
|vpcd| needs to be installed. It can be installed on Windows and Unix. On the
host computer, :command:`vpcd-config` prints a QR code to configure the Remote
Smart Card Reader. Scan the configuration with the bar code scanner of your
choice.

.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _OpenSC: https://github.com/OpenSC/OpenSC
.. _Android Studio: http://developer.android.com/sdk/installing/studio.html
.. _F-Droid: https://f-droid.org/repository/browse/?fdid=com.vsmartcard.remotesmartcardreader.app
.. _Android Open Source Project - Issue Tracker: https://code.google.com/p/android/issues/detail?id=76598
