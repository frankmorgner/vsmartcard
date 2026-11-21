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

*****************************************************
Supported Card Technologies and PC/SC APDU Interface
*****************************************************

ISO/IEC 14443-4 (ISO-DEP) / MIFARE DESFire
==========================================

Standard ISO/IEC 7816-4 APDUs and MIFARE DESFire ISO/IEC 7816-4 wrapped or
native frames are exchanged via Android's ``IsoDep`` interface. ATR generation
follows PC/SC Part 3 Section 3.1.3.2.3.1 (Table 3-5). In addition,
``IsoDepReader`` implements the following PC/SC Part 3 and ACS ACR122U reader
commands required by PC/SC middleware and tools such as ``libfreefare``:

+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| APDU / Command               | Specification / Origin             | Description                                                                                                  |
+==============================+====================================+==============================================================================================================+
| `FF CA 00 00 Le`             | PC/SC Part 3 Section 3.2.2.1.3 /   | **GET DATA (UID)**: Returns the anti-collision identifier (4-, 7-, or 10-byte UID), followed by `90 00`      |
|                              | ACS ACR122U Section 4.1            | (`6C XX` if `0 < Le < UID.length`; zero-padded + `62 82` if `Le > UID.length`).                              |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `FF CA 01 00 Le`             | PC/SC Part 3 Section 3.2.2.1.3 /   | **GET DATA (ATS Historical Bytes)**: Returns the historical bytes from the ISO/IEC 14443-4 Type A ATS        |
|                              | ACS ACR122U Section 4.1            | (without CRC) or Type B attribution data, followed by `90 00`.                                               |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `FF 00 00 00 Lc [Data]`      | ACS ACR122U Section 6.1            | **DIRECT TRANSMIT**: Strips the 5-byte ACR122U pseudo-APDU header and forwards `[Data]` directly to          |
|                              |                                    | `IsoDep.transceive()`, appending `90 00`.                                                                    |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `[Native DESFire Frame]`     | ACS ACR122U Section 7.2 /          | **DESFIRE NATIVE MODE**: Passes native MIFARE DESFire command frames to `IsoDep.transceive()`. If the card   |
|                              | `libfreefare` (`mifare_desfire.c`) | returns a 1-byte status code (e.g. `00` or `AF`), `90 00` is appended to satisfy the PC/SC 2-byte minimum.   |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+


MIFARE Classic and MIFARE Ultralight
====================================

MIFARE Classic (Mini, 1K, 4K, Plus 2K in Classic mode) and MIFARE Ultralight
(Ultralight, Ultralight C) are ISO/IEC 14443-3 Type A memory/storage cards
that do not natively process ISO/IEC 7816-4 APDUs. ``MifareClassicReader`` and
``MifareUltralightReader`` generate the 20-byte PC/SC Part 3 Section
3.1.3.2.3.2 (Table 3-6) Contactless Storage Card ATR
(``3B 8F 80 01 80 4F 0C A0 00 00 03 06 03 [C0] [C1] 00 00 00 00 [TCK]``) using
the Registered Card Names from PC/SC Part 3 Supplemental Section 2.1.2.2 (Table 3)
(``00 01`` = MIFARE Classic 1K, ``00 02`` = MIFARE Classic 4K,
``00 03`` = MIFARE Ultralight, ``00 26`` = MIFARE Mini,
``00 36`` = MIFARE Plus 2K, ``00 37`` = MIFARE Plus 4K,
``00 3A`` = MIFARE Ultralight C) and support both
PC/SC Part 3 / ACS ACR122U pseudo-APDUs (``CLA = 0xFF``) and native MIFARE
frames (used by ``libfreefare``).
Note on 2K Cards: NXP never manufactured a native MIFARE Classic 2K chip. 2K memory in
MIFARE Classic framing is exclusively MIFARE Plus (MF1PLUS60) in Security Level 1 (SL1).
In accordance with PC/SC Part 3 Supplemental Table 3, 2048-byte cards are identified
as MIFARE Plus SL1 2K (``00 36``), which also serves as the standard PC/SC representation
for clone chips reporting 2K memory.

+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| APDU / Command                           | Specification / Origin             | Description                                                                                      |
+==========================================+====================================+==================================================================================================+
| `FF CA 00 00 Le`                         | PC/SC Part 3 Section 3.2.2.1.3 /   | **GET DATA (UID)**: Returns the 4-byte or 7-byte UID, followed by `90 00`.                       |
|                                          | ACS ACR122U Section 4.1            |                                                                                                  |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF 82 KeyStruct KeyNr 06 [Key]`         | PC/SC Part 3 Section 3.2.2.1.4 /   | **LOAD KEYS** (MIFARE Classic): Loads a 6-byte Crypto1 key into volatile reader key slot         |
|                                          | ACS ACR122U Section 5.1            | `KeyNr` (`0x00`..`0x1F`). Volatile slots are pre-initialized to `FF FF FF FF FF FF` as an        |
|                                          |                                    | implementation convenience default matching card transport keys (NXP MF1S50YYX_V1 Sec 8.6.3).    |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF 86 00 00 05 01 00 [Blk] [KT] [KN]`   | PC/SC Part 3 Section 3.2.2.1.6 /   | **GENERAL AUTHENTICATE** (MIFARE Classic): Authenticates the sector containing `Blk` using key   |
|                                          | ACS ACR122U Section 5.2            | type `KT` (`0x60` = Key A, `0x61` = Key B) and key slot `KN` (`0x00`..`0x1F`).                   |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF 88 00 [Blk] [KT] [KN]`               | PC/SC Part 3 Section 3.2.2.1.5 /   | **AUTHENTICATE (Legacy)** (MIFARE Classic): 6-byte obsolete authentication command for `Blk`.    |
|                                          | ACS ACR122U Section 5.2            |                                                                                                  |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF B0 00 [Blk/Page] [Le]`               | PC/SC Part 3 Section 3.2.2.1.8 /   | **READ BINARY**: Reads block(s) within the authenticated sector on MIFARE Classic (defaults to    |
|                                          | ACS ACR122U Section 5.3            | 16 bytes for `Le = 00` or absent; slices requested byte counts) or page(s) on MIFARE Ultralight  |
|                                          |                                    | (`Le = 04` / `10`), plus `90 00`.                                                                |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF D6 00 [Blk/Page] [Lc] [Data]`        | PC/SC Part 3 Section 3.2.2.1.9 /   | **UPDATE BINARY**: Writes a 16-byte block on MIFARE Classic (`Lc = 10h` per ACR122U §5.4;        |
|                                          | ACS ACR122U Section 5.4            | multi-block writes disallowed to prevent sector-crossing corruption) or 4-byte page(s) on       |
|                                          |                                    | MIFARE Ultralight (`Lc` = multiple of 4), returning `90 00`.                                    |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF D7 00 [Blk] 05 [Op] [4B Val]`        | ACS ACR122U Section 5.5.1 /        | **VALUE BLOCK OPERATION** (MIFARE Classic): `Op = 00` formats/stores 32-bit signed MSB value     |
|                                          | NXP MF1S50YYX_V1 Section 8.6.2.1   | into `Blk`; `Op = 01`/`02` increments/decrements `Blk` by non-negative delta and transfers.      |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF D7 00 [SrcBlk] 02 03 [DstBlk]`       | ACS ACR122U Section 5.5.3 /        | **RESTORE VALUE BLOCK** (MIFARE Classic): Copies value block `SrcBlk` to `DstBlk` within the     |
|                                          | NXP MF1S50YYX_V1 Sec 12.4--12.5    | same sector via `RESTORE` (`C2h`) + `TRANSFER` (`B0h`), returning `90 00`.                       |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF B1 00 [Blk] 04`                      | ACS ACR122U Section 5.5.2          | **READ VALUE BLOCK** (MIFARE Classic): Validates the 16-byte value block at `Blk` and returns    |
|                                          |                                    | the 4-byte signed integer (`MSB..LSB`) followed by `90 00`.                                      |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `FF 00 00 00 Lc [Data]`                  | ACS ACR122U Section 6.1            | **DIRECT TRANSMIT**: Forwards `[Data]` to `transceive()` and appends `90 00`.                    |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+
| `60/61`, `30`, `A0`, `A2`, `C0..C2`, `B0`| NXP MF1S50YYX_V1 Sec 9.1, 12.1--5 /| **NATIVE MIFARE FRAMES**: Executes native MIFARE Classic (`60`/`61` Auth, `30` Read, `A0` Write, |
|                                          | NXP MF0ICU1 Sec 7.6.5, 7.6.7--8 /  | `C1`/`C0`/`C2` Inc/Dec/Restore, `B0` Transfer) and Ultralight (`30` Read 4 pages, `A2` Write     |
|                                          | `libfreefare` (`pcsc-henryk`)      | 1 page, `A0` Compatibility Write) frames directly via Android's NFC tech APIs.                   |
+------------------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------+


JIS X 6319-4 / FeliCa (NFC-F)
=============================

FeliCa cards do not natively use ISO/IEC 7816-4 APDUs. ``remote-reader``
therefore exposes a PC/SC-style APDU interface combining PC/SC Part 3 Storage
Card commands, Sony PaSoRi PC/SC extensions, and direct frame pass-through:

+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| APDU / Command               | Specification / Origin             | Description                                                                                                  |
+==============================+====================================+==============================================================================================================+
| `FF CA 00 00 00`             | PC/SC Part 3 Section 3.2.2.1.3     | **GET DATA (UID)**: Returns card UID (8-byte IDm for FeliCa), followed by `90 00`.                           |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `FF CA 01 00 00`             | Sony PaSoRi PC/SC extension        | **GET DATA (PMm)**: Returns the 8-byte FeliCa PMm, followed by `90 00`.                                      |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `FF CA 02 00 00`             | `remote-reader` extension          | **GET DATA (System Code)**: Returns the 2-byte FeliCa System Code, followed by `90 00`.                      |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `FF A4 00 01 02 [SC]`        | Sony PaSoRi PC/SC extension        | **SELECT SERVICE**: Selects active 16-bit FeliCa Service Code (little-endian) for subsequent read/update.    |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `FF B0 P1 P2 Le`             | PC/SC Part 3 Section 3.2.2.1.8 /   | **READ BINARY**: Reads 16-byte FeliCa block(s) at block number `(P1 << 8) | P2` from the active service      |
|                              | Sony PaSoRi PC/SC extension        | via FeliCa *Read Without Encryption* (`0x06`), followed by `90 00`.                                          |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `FF D6 P1 P2 Lc Data`        | PC/SC Part 3 Section 3.2.2.1.9 /   | **UPDATE BINARY**: Writes a 16-byte FeliCa block at block number `(P1 << 8) | P2` to active service          |
|                              | Sony PaSoRi PC/SC extension        | via FeliCa *Write Without Encryption* (`0x08`), returning `90 00`.                                           |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `FF 00 00 00 Lc [Data]`      | ACS ACR122U Direct Transmit        | **DIRECT TRANSMIT**: Strips 5-byte ACR122U pseudo-APDU header and forwards encapsulated FeliCa packet        |
|                              | convention                         | to `NfcF.transceive()`, followed by `90 00`.                                                                 |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+
| `[LEN] [CMD] ...`            | Native FeliCa packet format        | **RAW PACKET**: Accepts native FeliCa packet beginning with length byte (`LEN == apdu.length`,               |
|                              | (JIS X 6319-4 Section 6.2)         | JIS X 6319-4) and forwards it directly to `NfcF.transceive()`.                                               |
+------------------------------+------------------------------------+--------------------------------------------------------------------------------------------------------------+

References:

- `PC/SC Workgroup Part 3 Rev 2.01.09: Requirements for PC-Connected Interface Devices <https://pcscworkgroup.com/Download/Specifications/pcsc3_v2.01.09.pdf>`_, Section 3.1.3.2.3.1 (Table 3-5: Contactless Smart Cards ATR), Section 3.1.3.2.3.2 (Table 3-6: Contactless Storage Cards ATR), and Section 3.2.2.1 (Contactless Commands: Section 3.2.2.1.3 Get Data, Section 3.2.2.1.4 Load Keys, Section 3.2.2.1.5 Authenticate, Section 3.2.2.1.6 General Authenticate, Section 3.2.2.1.8 Read Binary, Section 3.2.2.1.9 Update Binary)
- `PC/SC Workgroup Part 3 Supplemental Document for Contactless ICCs <https://pcscworkgroup.com/Download/Specifications/pcsc3_v2.01.09_sup.pdf>`_, Section 2.1.1--2.1.2.2 (Tables 1--3: RID, Standard Byte, and Registered Card Names for Contactless Storage Cards)
- `ACS ACR122U Application Programming Interface (API) V2.04 <https://www.acs.com.hk/download-manual/419/API-ACR122U-2.04.pdf>`_, Section 3.1.1--3.1.2 (Tables 2--3: ATR format for PICCs), Section 4.1 (Get Data), Section 5.1--5.5 (MIFARE Classic & Ultralight Load Keys, Authentication, Read/Update Binary, Value Block Operations), Section 6.1 (Direct Transmit), Section 7.2 (MIFARE DESFire), and Section 7.3 (FeliCa)
- `NXP Semiconductors MF1S50YYX_V1: MIFARE Classic EV1 1K - Mainstream contactless smart card IC for fast and easy solution development, Rev. 3.2 (Doc. No. 279232, 2018-05-23) <https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf>`_, Section 8.6 (Memory organization), Section 8.6.2.1 (Table 4, Figure 8: Value blocks), Section 8.6.3 (Figure 9: Sector trailer / default keys), Section 9.1 (Table 9: MIFARE Classic command overview), Section 9.3 (Table 10: ACK and NAK), and Section 12.1--12.5 (Tables 20, 22, 24, 26, 28: MIFARE Classic commands)
- `NXP Semiconductors MF0ICU1: MIFARE Ultralight Contactless Single-Ticket IC Product Data Sheet, Rev. 3.9 (Doc. No. 028639, 2014-07-23) <https://www.nxp.com/docs/en/data-sheet/MF0ICU1.pdf>`_, Section 7.5 (Table 5: Memory organization), Section 7.6.5 (Table 10: READ), Section 7.6.7 (Table 12: WRITE), and Section 7.6.8 (Table 13: COMPATIBILITY WRITE)
- `libfreefare PC/SC backend (frankmorgner/libfreefare) <https://github.com/frankmorgner/libfreefare/tree/pcsc-henryk>`_ (``freefare.c``, ``mifare_classic.c``, ``mifare_ultralight.c``, ``mifare_desfire.c``)
- Sony Corporation, *SDK for NFC User's Manual - PC/SC Edition (Lite)* (Doc No. M579-J02), Section 3.2.6 "Proprietary/Extended Commands" (defines ``FF CA 01 00 00`` for PMm and ``FF A4 00 01 02 [SC]`` / ``FF B0`` for FeliCa service selection and unencrypted block access on Sony PaSoRi readers; see practical usage in `4d-plugin-scard <https://github.com/miyako/4d-plugin-scard>`_ and `OpenSC with Sony PaSoRi <https://qiita.com/gebo/items/cb2dd393170767852fb3>`_)



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
