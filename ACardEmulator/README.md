# Android Smart Card Emulator

The Android Smart Card Emulator allows the emulation of a contact-less smart card.
The emulator uses Android's HCE to fetch APDUs from a contact-less reader.
The app allows to process the Command APDUs either by delegating them to a
remote virtual smart card or by a built-in Java Card simulator. The response
APDUs are then returned to the smart card reader.

With the built-in Java Card runtime of [jCardSim](http://www.jcardsim.org/) the
app includes the following Applets:

- [Hello World Applet](https://github.com/licel/jcardsim/blob/master/src/main/java/com/licel/jcardsim/samples/HelloWorldApplet.java) (application identifier ``F000000001``)
- [OpenPGP Applet](https://developers.yubico.com/ykneo-openpgp/) (application identifier ``D2760001240102000000000000010000``)
- [OATH Applet](https://developers.yubico.com/ykneo-oath/) (application identifier ``A000000527210101``)
- [ISO Applet](http://www.pwendland.net/IsoApplet/) (application identifier ``F276A288BCFBA69D34F31001``)
- [GIDS Applet](https://github.com/vletoux/GidsApplet) (application identifier ``A000000397425446590201``)

The remote interface can be used together with the [Virtual Smart
Card](http://frankmorgner.github.io/vsmartcard/virtualsmartcard/README.html),
which allows emulating the following cards:

- Generic ISO-7816 smart card
- German electronic identity card (nPA)
- Electronic passport

Please refer to [our project's website](http://frankmorgner.github.io/vsmartcard/ACardEmulator/README.html) for more information.
