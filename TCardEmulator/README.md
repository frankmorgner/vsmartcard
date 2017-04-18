# Tizen Smart Card Emulator

The Tizen Smart Card Emulator allows the emulation of a contact-less smart
card.  The emulator uses Tizen's HCE to fetch APDUs from a contact-less reader.
The headless Tizen service delegates the Command APDUs via Samsung's Accessory
Protocol to the [Android Smart Card
Emulator](http://frankmorgner.github.io/vsmartcard/ACardEmulator/README.html).
The android app processes the commands and sends responses back to the
contact-less reader via the Tizen Smart Card Emulator.

Please refer to [our project's website](http://frankmorgner.github.io/vsmartcard/TCardEmulator/README.html) for more information.
