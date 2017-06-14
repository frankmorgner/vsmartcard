#
# Copyright (C) 2011 Frank Morgner, Dominik Oepen
#
# This file is part of virtualsmartcard.
#
# virtualsmartcard is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# virtualsmartcard is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# virtualsmartcard.  If not, see <http://www.gnu.org/licenses/>.
#

import atexit
import errno
import logging
import socket
import struct
import sys
from virtualsmartcard.ConstantDefinitions import MAX_EXTENDED_LE, MAX_SHORT_LE
from virtualsmartcard.SWutils import SwError, SW
from virtualsmartcard.SmartcardFilesystem import make_property
from virtualsmartcard.utils import C_APDU, R_APDU, hexdump, inttostring
from virtualsmartcard.CardGenerator import CardGenerator


class SmartcardOS(object):
    """Base class for a smart card OS"""

    def getATR(self):
        """Returns the ATR of the card as string of characters"""
        return ""

    def powerUp(self):
        """Powers up the card"""
        pass

    def powerDown(self):
        """Powers down the card"""
        pass

    def reset(self):
        """Performs a warm reset of the card (no power down)"""
        pass

    def execute(self, msg):
        """Returns response to the given APDU as string of characters

        :param msg: the APDU as string of characters
        """
        return ""

    def logAPDU(self, parsed, unparsed):
        if(self.logunparsed):
            logging.info("Unparsed APDU:\n%s", hexdump(unparsed));
        else:
            logging.info("Parsed APDU:\n%s", str(parsed))

class Iso7816OS(SmartcardOS):

    mf = make_property("mf",  "master file")
    SAM = make_property("SAM", "secure access module")

    def __init__(self, mf, sam, ins2handler=None, extended_length=False):
        self.mf = mf
        self.SAM = sam

        if not ins2handler:
            self.ins2handler = {
                    0x0c: self.mf.eraseRecord,
                    0x0e: self.mf.eraseBinaryPlain,
                    0x0f: self.mf.eraseBinaryEncapsulated,
                    0x2a: self.SAM.perform_security_operation,
                    0x20: self.SAM.verify,
                    0x22: self.SAM.manage_security_environment,
                    0x24: self.SAM.change_reference_data,
                    0x46: self.SAM.generate_public_key_pair,
                    0x82: self.SAM.external_authenticate,
                    0x84: self.SAM.get_challenge,
                    0x88: self.SAM.internal_authenticate,
                    0xa0: self.mf.searchBinaryPlain,
                    0xa1: self.mf.searchBinaryEncapsulated,
                    0xa4: self.mf.selectFile,
                    0xb0: self.mf.readBinaryPlain,
                    0xb1: self.mf.readBinaryEncapsulated,
                    0xb2: self.mf.readRecordPlain,
                    0xb3: self.mf.readRecordEncapsulated,
                    0xc0: self.getResponse,
                    0xca: self.mf.getDataPlain,
                    0xcb: self.mf.getDataEncapsulated,
                    0xd0: self.mf.writeBinaryPlain,
                    0xd1: self.mf.writeBinaryEncapsulated,
                    0xd2: self.mf.writeRecord,
                    0xd6: self.mf.updateBinaryPlain,
                    0xd7: self.mf.updateBinaryEncapsulated,
                    0xda: self.mf.putDataPlain,
                    0xdb: self.mf.putDataEncapsulated,
                    0xdc: self.mf.updateRecordPlain,
                    0xdd: self.mf.updateRecordEncapsulated,
                    0xe0: self.mf.createFile,
                    0xe2: self.mf.appendRecord,
                    0xe4: self.mf.deleteFile,
                    }
        else:
            self.ins2handler = ins2handler

        if extended_length:
            self.maxle = MAX_EXTENDED_LE
        else:
            self.maxle = MAX_SHORT_LE

        self.lastCommandOffcut = ""
        self.lastCommandSW = SW["NORMAL"]
        el = extended_length  # only needed to keep following line short
        tsft = Iso7816OS.makeThirdSoftwareFunctionTable(extendedLe=el)
        card_capabilities = self.mf.firstSFT + self.mf.secondSFT + tsft
        self.atr = Iso7816OS.makeATR(T=1, directConvention=True, TA1=0x13,
                                     histChars=chr(0x80) +
                                     chr(0x70 + len(card_capabilities)) +
                                     card_capabilities)

    def getATR(self):
        return self.atr

    @staticmethod
    def makeATR(**args):
        """Calculate Answer to Reset (ATR) and returns the bitstring.

            - directConvention (bool): Whether to use direct convention or
                                       inverse convention.
            - TAi, TBi, TCi (optional): Value between 0 and 0xff. Interface
                            Characters (for meaning see ISO 7816-3). Note that
                            if no transmission protocol is given, it is
                            automatically selected with T=max{j-1|TAj in args
                            OR TBj in args OR TCj in args}.
            - T (optional): Value between 0 and 15. Transmission Protocol.
                            Note that if T is set, TAi/TBi/TCi for i>T are
                            omitted.
            - histChars (optional): Bitstring with 0 <= len(histChars) <= 15.
                                    Historical Characters T1 to T15 (for
                                    meaning see ISO 7816-4).

        T0, TDi and TCK are automatically calculated.
        """
        # first byte TS
        if args["directConvention"]:
            atr = "\x3b"
        else:
            atr = "\x3f"

        if "T" in args:
            T = args["T"]
        else:
            T = 0

        # find maximum i of TAi/TBi/TCi in args
        maxTD = 0
        i = 15
        while i > 0:
            if ("TA" + str(i) in args or "TB" + str(i) in args or
                    "TC" + str(i) in args):
                maxTD = i-1
                break
            i -= 1

        if maxTD == 0 and T > 0:
            maxTD = 2

        # insert TDi into args (TD0 is actually T0)
        for i in range(0, maxTD+1):
            if i == 0 and "histChars" in args:
                args["TD0"] = len(args["histChars"])
            else:
                args["TD"+str(i)] = T

            if i < maxTD:
                args["TD"+str(i)] |= 1 << 7

            if "TA" + str(i+1) in args:
                args["TD"+str(i)] |= 1 << 4
            if "TB" + str(i+1) in args:
                args["TD"+str(i)] |= 1 << 5
            if "TC" + str(i+1) in args:
                args["TD"+str(i)] |= 1 << 6

        # initialize checksum
        TCK = 0

        # add TDi, TAi, TBi and TCi to ATR (TD0 is actually T0)
        for i in range(0, maxTD+1):
            atr = atr + "%c" % args["TD" + str(i)]
            TCK ^= args["TD" + str(i)]
            for j in ["A", "B", "C"]:
                if "T" + j + str(i+1) in args:
                    atr += "%c" % args["T" + j + str(i+1)]
                    # calculate checksum for all bytes from T0 to the end
                    TCK ^= args["T" + j + str(i+1)]

        # add historical characters
        if "histChars" in args:
            atr += args["histChars"]
            for i in range(0, len(args["histChars"])):
                TCK ^= ord(args["histChars"][i])

        # checksum is omitted for T=0
        if T > 0:
            atr += "%c" % TCK

        return atr

    @staticmethod
    def makeThirdSoftwareFunctionTable(commandChainging=False,
                                       extendedLe=False,
                                       assignLogicalChannel=0,
                                       maximumChannels=0):
        """
        Returns a byte according to the third software function table from the
        historical bytes of the card capabilities.
        """
        tsft = 0
        if commandChainging:
            tsft |= 1 << 7
        if extendedLe:
            tsft |= 1 << 6
        if assignLogicalChannel:
            if not (0 <= assignLogicalChannel and assignLogicalChannel <= 3):
                raise ValueError
            tsft |= assignLogicalChannel << 3
        if maximumChannels:
            if not (0 <= maximumChannels and maximumChannels <= 7):
                raise ValueError
            tsft |= maximumChannels
        return inttostring(tsft)

    def formatResult(self, seekable, le, data, sw, sm):
        if not seekable:
            self.lastCommandOffcut = data[le:]
            l = len(self.lastCommandOffcut)
            if l == 0:
                self.lastCommandSW = SW["NORMAL"]
            else:
                self.lastCommandSW = sw
                sw = SW["NORMAL_REST"] + min(0xff, l)
        else:
            if le > len(data):
                sw = SW["WARN_EOFBEFORENEREAD"]

        if le is not None:
            result = data[:le]
        else:
            result = data[:0]
        if sm:
            sw, result = self.SAM.protect_result(sw, result)

        return R_APDU(result, inttostring(sw)).render()

    @staticmethod
    def seekable(ins):
        if ins in [0xb0, 0xb1, 0xd0, 0xd1, 0xd6, 0xd7, 0xa0, 0xa1, 0xb2, 0xb3,
                   0xdc, 0xdd]:
            return True
        else:
            return False

    def getResponse(self, p1, p2, data):
        if not (p1 == 0 and p2 == 0):
            raise SwError(SW["ERR_INCORRECTP1P2"])

        return self.lastCommandSW, self.lastCommandOffcut

    def execute(self, msg):
        def notImplemented(*argz, **args):
            """
            If an application tries to use a function which is not implemented
            by the currently emulated smartcard we raise an exception which
            should result in an appropriate response APDU being passed to the
            application.
            """
            raise SwError(SW["ERR_INSNOTSUPPORTED"])

        try:
            c = C_APDU(msg)
        except ValueError as e:
            logging.warning(str(e))
            return self.formatResult(False, 0, "",
                                     SW["ERR_INCORRECTPARAMETERS"], False)

        self.logAPDU(parsed=c, unparsed=msg)

        # Handle Class Byte
        # {{{
        class_byte = c.cla
        SM_STATUS = None
        logical_channel = 0
        command_chaining = 0
        header_authentication = 0

        # Ugly Hack for OpenSC-explorer
        if(class_byte == 0xb0):
            logging.debug("Open SC APDU")
            SM_STATUS = "No SM"

        # If Bit 8,7,6 == 0 then first industry values are used
        if (class_byte & 0xE0 == 0x00):
            # Bit 1 and 2 specify the logical channel
            logical_channel = class_byte & 0x03
            # Bit 3 and 4 specify secure messaging
            secure_messaging = class_byte >> 2
            secure_messaging &= 0x03
            if (secure_messaging == 0x00):
                SM_STATUS = "No SM"
            elif (secure_messaging == 0x01):
                SM_STATUS = "Proprietary SM"  # Not supported ?
            elif (secure_messaging == 0x02):
                SM_STATUS = "Standard SM"
            elif (secure_messaging == 0x03):
                SM_STATUS = "Standard SM"
                header_authentication = 1
        # If Bit 8,7 == 01 then further industry values are used
        elif (class_byte & 0x0C == 0x0C):
            # Bit 1 to 4 specify logical channel. 4 is added, value range is
            # from four to nineteen
            logical_channel = class_byte & 0x0f
            logical_channel += 4
            # Bit 6 indicates secure messaging
            secure_messaging = class_byte >> 6
            if (secure_messaging == 0x00):
                SM_STATUS = "No SM"
            elif (secure_messaging == 0x01):
                SM_STATUS = "Standard SM"
            else:
                # Bit 8 is set to 1, which is not specified by ISO 7816-4
                SM_STATUS = "Proprietary SM"
        # In both cases Bit 5 specifies command chaining
        command_chaining = class_byte >> 5
        command_chaining &= 0x01
        # }}}

        sm = False
        try:
            if SM_STATUS == "Standard SM" or SM_STATUS == "Proprietary SM":
                c = self.SAM.parse_SM_CAPDU(c, header_authentication)
                logging.info("Decrypted APDU:\n%s", str(c))
                sm = True
            sw, result = self.ins2handler.get(c.ins, notImplemented)(c.p1,
                                                                     c.p2,
                                                                     c.data)
            answer = self.formatResult(Iso7816OS.seekable(c.ins),
                                       c.effective_Le, result, sw, sm)
        except SwError as e:
            logging.info(e.message)
            import traceback
            traceback.print_exception(*sys.exc_info())
            sw = e.sw
            result = ""
            answer = self.formatResult(False, 0, result, sw, sm)

        return answer

    def powerUp(self):
        self.mf.current = self.mf

    def reset(self):
        self.mf.current = self.mf


# sizeof(int) taken from asizof-package {{{
_Csizeof_short = len(struct.pack('h', 0))
# }}}


VPCD_CTRL_LEN = 1
VPCD_CTRL_OFF = 0
VPCD_CTRL_ON = 1
VPCD_CTRL_RESET = 2
VPCD_CTRL_ATR = 4


class VirtualICC(object):
    """
    This class is responsible for maintaining the communication of the virtual
    PCD and the emulated smartcard. vpicc and vpcd communicate via a socket.
    The vpcd sends command APDUs (which it receives from an application) to the
    vicc. The vicc passes these CAPDUs on to an emulated smartcard, which
    produces a response APDU. This RAPDU is then passed back by the vicc to
    the vpcd, which forwards it to the application.
    """

    def __init__(self, datasetfile, card_type, host, port,
                 readernum=None, ef_cardsecurity=None, ef_cardaccess=None,
                 ca_key=None, cvca=None, disable_checks=False, esign_key=None,
                 esign_ca_cert=None, esign_cert=None,
                 logginglevel=logging.INFO, logunparsed=False):
        from os.path import exists

        logging.basicConfig(level=logginglevel,
                            format="%(asctime)s  [%(levelname)s] %(message)s",
                            datefmt="%d.%m.%Y %H:%M:%S")

        self.cardGenerator = CardGenerator(card_type)

        # If a dataset file is specified, read the card's data groups from disk
        if datasetfile is not None:
            if exists(datasetfile):
                logging.info("Reading Data Groups from file %s.",
                             datasetfile)
                self.cardGenerator.readDatagroups(datasetfile)

        MF, SAM = self.cardGenerator.getCard()

        # Generate an OS object of the correct card_type
        if card_type == "iso7816" or card_type == "ePass":
            self.os = Iso7816OS(MF, SAM)
        elif card_type == "nPA":
            from virtualsmartcard.cards.nPA import NPAOS
            self.os = NPAOS(MF, SAM, ef_cardsecurity=ef_cardsecurity,
                            ef_cardaccess=ef_cardaccess, ca_key=ca_key,
                            cvca=cvca, disable_checks=disable_checks,
                            esign_key=esign_key, esign_ca_cert=esign_ca_cert,
                            esign_cert=esign_cert)
        elif card_type == "cryptoflex":
            from virtualsmartcard.cards.cryptoflex import CryptoflexOS
            self.os = CryptoflexOS(MF, SAM)
        elif card_type == "relay":
            from virtualsmartcard.cards.Relay import RelayOS
            self.os = RelayOS(readernum)
        elif card_type == "handler_test":
            from virtualsmartcard.cards.HandlerTest import HandlerTestOS
            self.os = HandlerTestOS()
        elif card_type == "belpic":
            from virtualsmartcard.cards.belpic import BelpicOS
            self.os = BelpicOS(MF, SAM)
        else:
            logging.warning("Unknown cardtype %s. Will use standard card_type \
                            (ISO 7816)", card_type)
            card_type = "iso7816"
            self.os = Iso7816OS(MF, SAM)
        self.type = card_type
        self.os.logunparsed = logunparsed

        # Connect to the VPCD
        self.host = host
        self.port = port
        if host:
            # use normal connection mode
            try:
                self.sock = self.connectToPort(host, port)
                self.sock.settimeout(None)
                self.server_sock = None
            except socket.error as e:
                logging.error("Failed to open socket: %s", str(e))
                logging.error("Is pcscd running at %s? Is vpcd loaded? Is a \
                              firewall blocking port %u?", host, port)
                sys.exit()
        else:
            # use reversed connection mode
            try:
                local_ip = [(s.connect(('8.8.8.8', 53)), s.getsockname()[0], s.close()) for s in [socket.socket(socket.AF_INET, socket.SOCK_DGRAM)]][0][1]
                custom_url = 'vicc://%s:%d' % (local_ip, port)
                print('VICC hostname:  %s' % local_ip);
                print('VICC port:      %d' % port)
                print('On your NFC phone with the Android Smart Card Emulator app scan this code:')
                try:
                    import qrcode
                    qr = qrcode.QRCode()
                    qr.add_data(custom_url)
                    qr.print_ascii()
                except ImportError:
                    print('https://api.qrserver.com/v1/create-qr-code/?data=%s' % custom_url)
                (self.sock, self.server_sock, host) = self.openPort(port)
                self.sock.settimeout(None)
            except socket.error as e:
                logging.error("Failed to open socket: %s", str(e))
                logging.error("Is pcscd running? Is vpcd loaded and in \
                              reversed connection mode? Is a firewall \
                              blocking port %u?", port)
                sys.exit()

        logging.info("Connected to virtual PCD at %s:%u", host, port)

        atexit.register(self.stop)

    @staticmethod
    def connectToPort(host, port):
        """
        Open a connection to a given host on a given port.
        """
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        return sock

    @staticmethod
    def openPort(port):
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind(('', port))
        server_socket.listen(0)
        logging.info("Waiting for vpcd on port " + str(port))
        (client_socket, address) = server_socket.accept()
        return (client_socket, server_socket, address[0])

    def __sendToVPICC(self, msg):
        """ Send a message to the vpcd """
        self.sock.sendall(struct.pack('!H', len(msg)) + msg)

    def __recvFromVPICC(self):
        """ Receive a message from the vpcd """
        # receive message size
        while True:
            try:
                sizestr = self.sock.recv(_Csizeof_short)
            except socket.error as e:
                if e.errno == errno.EINTR:
                    continue
            break
        if len(sizestr) == 0:
            logging.info("Virtual PCD shut down")
            raise socket.error
        size = struct.unpack('!H', sizestr)[0]

        # receive and return message
        if size:
            while True:
                try:
                    msg = self.sock.recv(size)
                except socket.error as e:
                    if e.errno == errno.EINTR:
                        continue
                break
            if len(msg) == 0:
                logging.info("Virtual PCD shut down")
                raise socket.error
        else:
            msg = None

        return size, msg

    def run(self):
        """
        Main loop of the vpicc. Receives command APDUs via a socket from the
        vpcd, dispatches them to the emulated smartcard and sends the resulting
        respsonse APDU back to the vpcd.
        """
        while True:
            try:
                (size, msg) = self.__recvFromVPICC()
            except socket.error as e:
                if not self.host:
                    logging.info("Waiting for vpcd on port " + str(self.port))
                    (self.sock, address) = self.server_sock.accept()
                    continue
                else:
                    sys.exit()

            if not size:
                logging.warning("Error in communication protocol (missing \
                                size parameter)")
            elif size == VPCD_CTRL_LEN:
                if msg == chr(VPCD_CTRL_OFF):
                    logging.info("Power Down")
                    self.os.powerDown()
                elif msg == chr(VPCD_CTRL_ON):
                    logging.info("Power Up")
                    self.os.powerUp()
                elif msg == chr(VPCD_CTRL_RESET):
                    logging.info("Reset")
                    self.os.reset()
                elif msg == chr(VPCD_CTRL_ATR):
                    self.__sendToVPICC(self.os.getATR())
                else:
                    logging.warning("unknown control command")
            else:
                if size != len(msg):
                    logging.warning("Expected %u bytes, but received only %u",
                                    size, len(msg))

                answer = self.os.execute(msg)
                logging.info("Response APDU (%d Bytes):\n%s\n", len(answer),
                             hexdump(answer))
                self.__sendToVPICC(answer)

    def stop(self):
        self.sock.close()
        if self.server_sock:
            self.server_sock.close()
