# PC/SC Relay

> **Relay a smart card to a contactless interface**
> **License:** GPL version 3
> **Tested Platforms:** Windows, Linux (Debian, Ubuntu, OpenMoko)

Welcome to PC/SC Relay. The purpose of PC/SC Relay is to relay a smart
card using an contact-less interface. Currently the following contact-less
**emulators** are supported:

- [Hardware supported by libnfc](http://nfc-tools.org/index.php?title=Devices_compatibility_matrix)
- [OpenPICC](http://www.openpcd.org/OpenPICC)
- [Android Smart Card Emulator](https://frankmorgner.github.io/vsmartcard/ACardEmulator/README.html)

Command APDUs are received with the contact-less interface and relayed. The
Response APDUs are then sent back via RFID. The contact-less data will be
relayed to one of the following **connectors**:

- to a *real* smart card inserted into one of the systems' smart card readers.
  The smart card reader must be accessible with PC/SC. The smart card may be
  contact-based *or* contact-less.
- to a [ Virtual Smart Card](https://frankmorgner.github.io/vsmartcard/virtualsmartcard/README.html) that directly connects to `pcsc-relay`. The virtual
  smart card's native interface is used and (despite its name) PC/SC Relay
  does not need to access PC/SC in this case.

```text
     ┌────────┐          ┌──────────┐          ┌─────────┐
     │Emulator│          │pcsc-relay│          │Connector│
     └───┬────┘          └────┬─────┘          └────┬────┘
         │    Command APDU    │                     │     
         │ ───────────────────>                     │     
         │                    │ ───────────────────>│     
         │                    │ <───────────────────│     
         │    Response APDU   │                     │     
         │ <───────────────────                     │     
     ┌───┴────┐          ┌────┴─────┐          ┌────┴────┐
     │Emulator│          │pcsc-relay│          │Connector│
     └────────┘          └──────────┘          └─────────┘
```

With PC/SC Relay you can relay a contact-less or contact based smart card
over a long distance. Also you can use it in combination with the `vicc`
to completely emulate an ISO/IEC 14443 smart card.

![Debug, Analyze and Emulate with PC/SC Relay](doc/tikz-115ce7f91e19ed28b7af35a9f18aa096148209ea.svg)

PC/SC Relay has the following dependencies:

- PC/SC middleware

> [!NOTE]
> This software can actually be used in a relay attack allowing full access
> to the card. We discussed the impact especially on the [Relay attack against the German ID card](http://media.ccc.de/browse/congress/2010/27c3-4297-de-die_gesamte_technik_ist_sicher.html),
> but it generally concerns *all* contact-less smart cards.

## Download

You can find the latest release of PC/SC Relay on [Github](https://github.com/frankmorgner/vsmartcard/releases). Older releases are
still available on [Sourceforge](http://sourceforge.net/projects/vsmartcard/files).

Alternatively, you can clone our git repository:

```sh
git clone https://github.com/frankmorgner/vsmartcard.git
cd vsmartcard
git submodule update --init --recursive
```

## Installation

### Installation on Linux, Unix and similar

The PC/SC Relay uses the GNU Build System to compile and install. If you are
unfamiliar with it, please have a look at `INSTALL`. If you can not find
it, you are probably working bleeding edge in the repository. To generate the
missing standard auxiliary files you need to additionally install `libtool` and
`pkg-config` and run the following command in `pcsc-relay`:

```sh
autoreconf --verbose --install
```

To configure (`configure --help` lists possible options), build and
install the PC/SC Relay now do the following:

```sh
./configure --sysconfdir=/etc
make
make install
```

### Hints on libnfc

Here is an example of how to get the standard
installation of libnfc:

```sh
PREFIX=/tmp/install
LIBNFC=libnfc
git clone https://github.com/nfc-tools/libnfc $LIBNFC
cd $LIBNFC
autoreconf -i
# See `./configure --help` for enabling support of additional hardware
./configure --prefix=$PREFIX
make
make install
```

Building PC/SC Relay with libnfc is done best using `pkg-config`. The file
`libnfc.pc` should be located in `$INSTALL/lib/pkgconfig`. Here is how to
configure PC/SC Relay to use it:

```sh
./configure PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig
```

### Hints on Android Smart Card Emulator

The Android Smart Card Emulator is build around the host card emulation mode of
Android 4.4 and later. This mode activates the app if the terminal issues a
SELECT command with one of the app's application identifiers. By default, the
app only registers for the AIDs for which it has a built-in emulator (see
`ACardEmulator/app/src/main/res/xml/aid_list.xml`).

If used together with PC/SC Relay, you need to change add AIDs to match the
applications on the relayed card. Otherwise the app will not be activated when
it should relay command APDUs to PC/SC Relay.

Modify the Smart Card Emulator settings to use `Remote Virtual Smart Card` as
`Smart Card Emulator`. Now start `pcsc-relay` by specifying
usage of the vpcd emulator:

```sh
pcsc-relay --emulator vpcd
```

In the app, change the `VICC Hostname` and `VICC Port` to
match the location where `pcsc-relay` is waiting for an incoming
connection. When the app receives a SELECT command to one of the configured
AIDs, it will connect to `pcsc-relay`, which will then relay the
command for processing.

Compiling and installing Android Smart Card Emulator is covered in its
installation section.

### Hints on PC/SC middleware

A PC/SC middleware is included by default in most modern operating systems. On
Unix-like systems (Linux, OS X, Sun OS) it is realized by [PCSC-Lite](https://pcsclite.apdu.fr/). To
compile PC/SC Relay you will need to install the PCSC-Lite headers from
your distribution.

Windows also ships with a PC/SC middleware in form of the Winscard module.
PC/SC Relay can be (cross) compiled with MinGW-w64. Also, Microsoft's
developement environment Visual Studio includes all necessary data for building
PC/SC Relay.

## Usage

```
Usage: pcsc-relay [OPTION]...
Relay a smart card to a contactless interface

Using an contact-less interface (currently OpenPICC or libnfc) pcsc-relay
receives command APDUs which are forwared to an existing smart card via PC/SC.
pcsc-relay sends the response APDU back to the contact-less interface.

  -h, --help                  Print help and exit
  -V, --version               Print version and exit
  -e, --emulator=ENUM         Contact-less emulator backend  (possible
                                values="libnfc", "vpcd", "openpicc"
                                default=`libnfc')
  -c, --connector=ENUM        Smart card connector backend  (possible
                                values="pcsc", "vicc" default=`pcsc')
  -f, --foreground            Stay in foreground  (default=on)
  -v, --verbose               Use (several times) to be more verbose

PC/SC connector:
  -r, --reader=INT            Number of the PC/SC reader to use (-1 for
                                autodetect)  (default=`-1')

Virtual Smart Card connector:
  -p, --vpcd-port=INT         Port for communicating virtual smart card
                                (default=`35963')
  -n, --vpcd-hostname=STRING  Hostname for connecting to virtual smart card
                                (default=`wait for an incoming connection')

Virtual Smart Card Reader emulator:
  -P, --vicc-port=INT         Port to listen for virtual smart card reader
                                (default=`35963')
  -N, --vicc-hostname=STRING  Hostname for connecting to virtual smart card
                                reader  (default=`wait for an incoming
                                connection')
  -A, --vicc-atr=STRING       ATR  (default=`3B80800101')

Report bugs to https://github.com/frankmorgner/vsmartcard/issues

Written by Frank Morgner and Dominik Oepen
```

Below we explain what option to choose for the emulator which receives a
command APDU and transmits a response APDU back to the terminal:

| Option | `--emulator` |
|--------|--------------|
| Emulation hardware supported via libnfc | `libnfc` |
| Emulation with OpenPICC | `openpicc` |
| Android Smart Card Emulator | `vpcd` |
| Virtual Smart Card | `vpcd` |

Below we explain what option to choose for the connector which calculates
a response APDU from a given command APDU:

| Option | `--connector` |
|--------|---------------|
| Contact-based Smart Card in PC/SC Reader | `pcsc` |
| Contact-less Smart Card in PC/SC Reader | `pcsc` |
| Contact-less Smart Card in Remote Smart Card Reader | `vicc` |
| Virtual Smart Card | `vicc` |

## Question

Do you have questions, suggestions or contributions? Feedback of any kind is
more than welcome! Please use our [project trackers](https://github.com/frankmorgner/vsmartcard/issues).
