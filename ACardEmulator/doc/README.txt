.. highlight:: sh

.. |HCE| replace:: :abbr:`HCE (host card emulation)`

.. _acardemulator:

################################################################################
Android Smart Card Emulator
################################################################################

.. sidebar:: Summary

    Use an Android phone as contact-less smart card

    :Author:
        `Frank Morgner <morgner@informatik.hu-berlin.de>`_
    :License:
        GPL version 3
    :Tested Platform:
        Android, CyanogenMod

The Android Smart Card Emulator allows the emulation of a contact-less smart card.
The emulator uses Android's |HCE| to fetch APDUs from a contact-less reader and
delegate them to Java Card Applets. The app includes the Java Card simulation
runtime of jCardSim_ as well as the following Java Card applets:

- `Hello World applet`_ (application identifier ``F000000001``)
- `OpenPGP applet`_ (application identifier ``D2760001240102000000000000010000``)
- `OATH applet`_ (application identifier ``A000000527210101``)
- `ISO applet`_ (application identifier ``F276A288BCFBA69D34F31001``)

.. tikz:: Simulate a contact-less smart card with Android Smart Card Emulator
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows
 
    \input{%(wd)s/bilder/tikzstyles.tex}
    \node (reader) {\includegraphics[width=3cm]{%(wd)s/bilder/my_cardreader.pdf}};
    \node [below=0cm of reader, kleiner] {Contact-less Reader};
    \node (phone) [right=1cm of reader] {\includegraphics[width=3cm]{%(wd)s/bilder/smartphone.pdf}};
    \node (app) [at=(phone.center)] {\includegraphics[width=2.8cm, height=4.9cm]{%(wd)s/bilder/ACardEmulator.png}};

    \begin{pgfonlayer}{background}
        \draw [rfid] (reader.center) -- (phone.west) ;
    \end{pgfonlayer}

The Android Smart Card Emulator has the following dependencies:

- NFC hardware built into the smartphone for |HCE|
- Android 4.4 "KitKat" (or newer) or CyanogenMod 11 (or newer)

For emulating a contact-less smart card with a desktop or notebook, have a look at :ref:`pcsc-relay`.


********************
Download and Install
********************

To manually compile the app you need to fetch the sources and initialize the
submodules::

    git clone https://github.com/frankmorgner/vsmartcard.git
    cd vsmartcard
    # fetch the applets that are in the submodules
    git submodule init
    git submodule update

We use `Android Studio`_ to build and deploy the application. Use
:menuselection:`File --> Open` to select :file:`vsmartcard/ACardEmulator`.
Attach your smartphone and choose :menuselection:`Run --> Run 'app'`.


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _jCardSim: http://www.jcardsim.org/
.. _Hello World Applet: https://github.com/licel/jcardsim/blob/master/src/main/java/com/licel/jcardsim/samples/HelloWorldApplet.java
.. _OpenPGP Applet: https://developers.yubico.com/ykneo-openpgp/
.. _OATH Applet: https://developers.yubico.com/ykneo-oath/
.. _ISO Applet: http://www.pwendland.net/IsoApplet/
.. _Android Studio: http://developer.android.com/sdk/installing/studio.html
