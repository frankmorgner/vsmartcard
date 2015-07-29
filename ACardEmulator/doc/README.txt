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

- `Hello World Applet`_ (application identifier ``F000000001``)
- `OpenPGP Applet`_ (application identifier ``D2760001240102000000000000010000``)
- `OATH Applet`_ (application identifier ``A000000527210101``)
- `ISO Applet`_ (application identifier ``F276A288BCFBA69D34F31001``)
- `MUSCLE Applet`_ (application identifier ``A00000000101``)

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

Please note that the currently emulated applets are verifying the PIN by
transmitting it without any protection between card and terminal. You may want
to have a look at `Erik Nellesson's
<http://sar.informatik.hu-berlin.de/research/publications/SAR-PR-2014-08/SAR-PR-2014-08_.pdf>`_
`Virtual Keycard`_,which uses the PACE protocol for PIN verification.


.. _acardemulator_install:

********************
Download and Install
********************

The Android Smart Card Emulator is available on F-Droid_.

.. qr code generated via http://www.qrcode-monkey.de

.. icon generated via https://romannurik.github.io/AndroidAssetStudio/icons-launcher.html#foreground.type=clipart&foreground.space.trim=0&foreground.space.pad=0.25&foreground.clipart=res%2Fclipart%2Ficons%2Fdevice_nfc.svg&foreColor=fdd017%2C0&crop=0&backgroundShape=hrect&backColor=ffffff%2C100&effects=shadow

.. image:: acardemu-qrcode.png
    :target: https://f-droid.org/repository/browse/?fdid=com.vsmartcard.acardemulator
    :alt: Android Smart Card Emulator on F-Droid
    :width: 265px
    :height: 265px

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
.. _MUSCLE Applet: https://github.com/martinpaljak/MuscleApplet/tree/d005f36209bdd7020bac0d783b228243126fd2f8
.. _Virtual Keycard: https://github.com/eriknellessen/Virtual-Keycard
.. _F-Droid: https://f-droid.org/repository/browse/?fdid=com.vsmartcard.remotesmartcardreader.app
.. _Android Studio: http://developer.android.com/sdk/installing/studio.html
