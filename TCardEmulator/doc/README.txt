.. highlight:: sh

.. |HCE| replace:: :abbr:`HCE (host card emulation)`

.. _tcardemulator:

################################################################################
Tizen Smart Card Emulator
################################################################################

.. sidebar:: Use a Tizen smartwatch as contact-less smart card

    :License:
        GPL version 3
    :Tested Platform:
        Tizen (Samsung Gear S2)

The Tizen Smart Card Emulator allows the emulation of a contact-less smart card.
The emulator uses Tizen's |HCE| to fetch APDUs from a contact-less reader.
The headless Tizen service delegates the Command APDUs via Samsung's Accessory
Protocol to the :ref:`acardemulator`. The android app processes the commands and
sends responses back to the contact-less reader via the Tizen Smart Card
Emulator.

You may also attach your own simulation by using the Samsung Accessory Protocol
for communicating with the tizen service.

.. tikz:: Simulate a contact-less smart card with Android Smart Card Emulator
    :stringsubst:
    :libs: arrows, calc, fit, patterns, plotmarks, shapes.geometric, shapes.misc, shapes.symbols, shapes.arrows, shapes.callouts, shapes.multipart, shapes.gates.logic.US, shapes.gates.logic.IEC, er, automata, backgrounds, chains, topaths, trees, petri, mindmap, matrix, calendar, folding, fadings, through, positioning, scopes, decorations.fractals, decorations.shapes, decorations.text, decorations.pathmorphing, decorations.pathreplacing, decorations.footprints, decorations.markings, shadows

    \input{$wd/bilder/tikzstyles.tex}
    \node (reader) {\includegraphics[width=3cm]{$wd/bilder/my_cardreader.pdf}};
    \node [below=0cm of reader, kleiner] {Contact-less Reader};
    \node (gears2) [right=1cm of reader] {\includegraphics[width=3cm]{$wd/bilder/gears2.jpg}};
    \node (phone) [right=-.5cm of gears2] {\includegraphics[width=3cm]{$wd/bilder/smartphone.pdf}};
    \node (app) [at=(phone.center)] {\includegraphics[width=2.8cm, height=4.9cm]{$wd/bilder/ACardEmulator.png}};

    \begin{pgfonlayer}{background}
        \draw [rfid] (reader.center) -- (gears2.west) ;
    \end{pgfonlayer}


********************
Download and Install
********************

To manually compile the app you need to fetch the sources and initialize the
submodules::

    git clone https://github.com/frankmorgner/vsmartcard.git

We use `Tizen SDK`_ to build and deploy the application. Use
:menuselection:`Import...` to select :menuselection:`Tizen --> Tizen Project`.
In the next dialog choose :file:`Tizen/TCardEmulator`. To be able to build and
install the Tizen service on the smartwatch, you need to `install the
appropriate SDK extensions and register as app developer`_.


.. include:: questions.txt


********************
Notes and References
********************

.. target-notes::

.. _Tizen SDK: https://developer.tizen.org/development/tools/download
.. _`install the appropriate SDK extensions and register as app developer`: https://developer.tizen.org/community/tip-tech/tizen-sdk-install-guide-certificate-extensions-included
