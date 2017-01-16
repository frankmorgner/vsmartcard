.. highlight:: sh

.. |vpcd| replace:: :abbr:`vpcd (virtual smart card reader)`

.. _remote-reader:

################################################################################
Remote Smart Card Reader
################################################################################

.. sidebar:: Use an Android phone as contact-less smart card reader

    :License:
        GPL version 3
    :Tested Platform:
        Android, CyanogenMod

Allow a host computer to use the smartphone's NFC hardware as contact-less
smartcard reader. On the host computer a special smart card driver, |vpcd|, must
be installed. The app establishes a connection to |vpcd| over the network when a
contact-less card is detected.


.. tikz:: Remote Smart Card Reader used to access a contact-less card
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{$wd/bilder/tikzstyles.tex}
	\node (pcsc) [klein, aktivbox, inner xsep=.75cm, align=center] {PC/SC\\Framework};
    \node (sca) [aktivbox, klein, left=.75cm of pcsc, align=center] {Smart Card\\Application};
    \node (vpcd) [box, at=(pcsc.east), kleiner] {\texttt{vpcd}};
    \node (phone) [right=2cm of vpcd] {\includegraphics[width=3cm]{$wd/bilder/smartphone.pdf}};
    \node (app) [at=(phone.center)] {\includegraphics[width=2.8cm, height=4.9cm]{$wd/bilder/remote-reader.png}};
    \node (card) [right=0cm of phone, rotate=45] {\includegraphics[width=2cm]{$wd/bilder/smartcard.pdf}};

    \begin{pgfonlayer}{background}
        \node (box) [box, fit=(pcsc) (sca) (vpcd), inner ysep=1.5em] {};
        \node [at=(box.north west)] {\includegraphics[width=1cm]{$wd/bilder/computer-tango.pdf}};
        \path[linie]
        (sca) edge (pcsc)
        (vpcd) edge node {\includegraphics[width=1.5cm]{$wd/bilder/simplecloud.pdf}} (app)
        ;
        \draw [rfid] (phone.center) -- (card.center) ;
    \end{pgfonlayer}

The Remote Smart Card Reader has the following dependencies:

- NFC hardware built into the smartphone
- Android 4.4 "KitKat" or CyanogenMod 11 (or newer)
- permissions for a data connection (communication with |vpcd|) and for using
  NFC (communication to the card); scanning the configuration via QR code
  requires permission to access the camera
- |vpcd| :ref:`installed on the host computer<vicc_install>`

For remotely accessing a traditional smart card reader on one computer from an
other computer, the :ref:`vicc` in relay mode can be used:

.. tikz:: Virtual Smart Card used in relay mode to remotely access a card
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{$wd/bilder/tikzstyles.tex}
	\node (pcsc) [klein, aktivbox, inner xsep=.75cm, align=center] {PC/SC\\Framework};
    \node (sca) [aktivbox, klein, left=.75cm of pcsc, align=center] {Smart Card\\Application};
    \node (vpcd) [box, at=(pcsc.east), kleiner] {\texttt{vpcd}};
	\node (pcsc2) [klein, aktivbox, inner xsep=.75cm, align=center, below=2.5cm of vpcd] {PC/SC\\Framework};
    \node (vicc) [aktivbox, left=1cm of pcsc2, kleiner] {\texttt{vicc -t relay}};
    \node (reader) [right=1cm of pcsc2] {\includegraphics[width=2cm]{$wd/bilder/my_cardreader.pdf}};
    \node (card) [at=(reader.east), rotate=45] {\includegraphics[width=2cm]{$wd/bilder/smartcard.pdf}};

    \begin{pgfonlayer}{background}
        \node (box) [box, fit=(pcsc) (sca) (vpcd), inner ysep=1.5em] {};
        \node (box2) [box, fit=(vicc) (pcsc2), inner ysep=1.5em] {};
        \node [at=(box.north west)] {\includegraphics[width=1cm]{$wd/bilder/computer-tango.pdf}};
        \node [at=(box2.north west)] {\includegraphics[width=1cm]{$wd/bilder/computer-tango.pdf}};
        \path[linie]
        (sca) edge (pcsc)
        (vpcd) edge [out=south, in=north] node {\includegraphics[width=1.5cm]{$wd/bilder/simplecloud.pdf}} (vicc)
        (vicc) edge (pcsc2)
        (pcsc2) edge [usb] (reader.center)
        ;
    \end{pgfonlayer}



.. include:: relay-note.txt


********************
Download and Install
********************

The Remote Smart Card Reader is available on F-Droid_.

.. qr code generated via http://www.qrcode-monkey.de

.. icon generated via https://romannurik.github.io/AndroidAssetStudio/icons-launcher.html#foreground.type=clipart&foreground.space.trim=0&foreground.space.pad=0.25&foreground.clipart=res%2Fclipart%2Ficons%2Fnotification_tap_and_play.svg&foreColor=fdd017%2C0&crop=0&backgroundShape=hrect&backColor=ffffff%2C100&effects=shadow

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
