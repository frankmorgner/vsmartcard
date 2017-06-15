# Virtual Smart Card

Virtual Smart Card emulates a smart card and makes it accessible through PC/SC.
Currently the Virtual Smart Card supports the following types of smart cards:

- Generic ISO-7816 smart card including secure messaging
- German electronic identity card (nPA) with complete support for EAC
  (PACE, TA, CA)
- Electronic passport (ePass/MRTD) with support for BAC
- Cryptoflex smart card (incomplete)
- Belgian electronic ID card (WIP, this fork)
      
The vpcd is a smart card reader driver for [PCSC-Lite](http://pcsclite.alioth.debian.org/) and the windows smart
card service. It allows smart card applications to access the vpicc through
the PC/SC API.  By default vpcd opens slots for communication with multiple
vpicc's on localhost on port 35963 and port 35964. But the |vpicc| does not
need to run on the same machine as the vpcd, they can connect over the
internet for example.

Although the Virtual Smart Card is a software emulator, you can use
[pcsc-relay](http://frankmorgner.github.io/vsmartcard/pcsc-relay/README.html)
to make it accessible to an external contact-less smart card reader.

Please refer to [our project's website](http://frankmorgner.github.io/vsmartcard/virtualsmartcard/README.html) for more information.
