# Creating a Virtual Smart Card

`vpcd` (virtual smart card reader) communicates over a socket with `vpicc` (virtual smart card) usually on port `0x8C7B` (configurable via `/etc/reader.conf.d/vpcd`). So you can connect virtually any program to the virtual smart card reader, as long as you respect the following protocol:

| `vpcd` Length | `vpcd` Command | `vpicc` Length | `vpicc` Response |
|---------------|----------------|----------------|------------------|
| `0x00 0x01`   | `0x00` (Power Off) |            | (No Response)    |
| `0x00 0x01`   | `0x01` (Power On)  |            | (No Response)    |
| `0x00 0x01`   | `0x02` (Reset)     |            | (No Response)    |
| `0x00 0x01`   | `0x04` (Get ATR)   | `0xXX 0xXX`| (ATR)            |
| `0xXX 0xXX`   | (APDU)             | `0xXX 0xXX`| (R-APDU)         |

The communication is initiated by `vpcd`. First the length of the data (in network byte order, i.e. big endian) is sent followed by the data itself.

## Examples

### Implementing a ISO 7816 like Smart Card

`vpicc` includes an emulation of a card acting according to ISO 7816. This includes all standard commands for file management and secure messaging.

Let's assume we want to create a cryptoflex card, that acts mostly according to ISO 7816. In this example we only want to add little things that differ from ISO 7816. But as for most complex software you need to know where you need to hook into. Here we only want to give an overview to the design.

Back to the cryptoflex example. `VirtualICC` provides the connection to the virtual smart card reader. It fetches an APDU and other requests from the `vpcd`. In `VirtualICC` an APDU is only a buffer that is forwarded to the smart card OS. First we modify `VirtualICC` to recognize a new type `"cryptoflex"` and to load `CryptoflexOS`. The `CardGenerator` is used to create a file system and a SAM specific to the cryptoflex (we come back to this later).

> **Note:** See [`VirtualICC.__init__`](https://github.com/frankmorgner/vsmartcard/blob/master/virtualsmartcard/src/vpicc/virtualsmartcard/VirtualSmartcard.py#426)

Responses from our cryptoflex card look the same as for the 7816 card. But when a command was successful (or not) there is a little difference in what is returned. So we need to edit `CryptoflexOS.formatResult`, which is called to encode the status bytes (SW1 and SW2) and the resulting data.

> **Note:** See [`CryptoflexOS.formatResult`](https://github.com/frankmorgner/vsmartcard/blob/master/virtualsmartcard/src/vpicc/virtualsmartcard/cards/cryptoflex.py#L64)

Note that this also requires some insight knowledge about how `Iso7816OS` works.

The previously created SAM handles keys, encryption, secure messaging and so on (we will not go into more details here). The file system creates, selects and reads contents of files or directories. File handling for our cryptoflex card is similar to ISO 7816, but the meaning of P1, P2 and the APDU data is completely different when creating a file on the smart card. So we derive `CryptoflexMF` from `SmartcardFilesystem.MF` and modify `CryptoflexMF.create` to our needs.

> **Note:** See [`CryptoflexMF.create`](https://github.com/frankmorgner/vsmartcard/blob/master/virtualsmartcard/src/vpicc/virtualsmartcard/cards/cryptoflex.py#L188)

As you can see it is quite simple to extend the virtual smart card to your requirements. Simply overwrite those functions, that differ from ISO 7816. But as said before, the virtual smart card is quite complex and you might have to read some documentation or even source code to find out where it's best to do your modifications...

### Implementing an Other Type of Card

If you have a card entirely different to ISO 7816, you surely want to avoid all magic that is done while parsing a buffer (an APDU). As example we will connect to an other smart card using PC/SC and forward it to `vpcd`. 

As before with the cryptoflex card, we let `VirtualICC` recognize the new type `"relay"`. `RelayOS` overwrites all main functions from the template `SmartcardOS`. Its functions correspond to the commands sent by `vpcd`. If you know how to use [pyscard](http://pyscard.sourceforge.net/) then the rest is pretty straight forward, but see yourself...

> **Note:** See [`RelayOS`](https://github.com/frankmorgner/vsmartcard/blob/master/virtualsmartcard/src/vpicc/virtualsmartcard/cards/Relay.py#L28)
