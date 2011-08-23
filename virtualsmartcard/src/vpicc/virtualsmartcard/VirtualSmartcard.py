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

from virtualsmartcard.ConstantDefinitions import MAX_EXTENDED_LE, MAX_SHORT_LE
from virtualsmartcard.SWutils import SwError, SW
from virtualsmartcard.SmartcardFilesystem import make_property
from virtualsmartcard.utils import C_APDU, R_APDU, hexdump, inttostring
import CardGenerator

import socket, struct, sys, signal, atexit, smartcard, logging


class SmartcardOS(object): # {{{
    """Base class for a smart card OS"""

    mf  = make_property("mf",  "master file")
    SAM = make_property("SAM", "secure access module")

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
        
        msg -- the APDU as string of characters
        """
        return ""
# }}}

class Iso7816OS(SmartcardOS): # {{{ 
    def __init__(self, mf, sam, ins2handler=None, extended_length=False):
        self.mf = mf
        self.SAM = sam

        #if self.mf == None and self.SAM == None:
        #    self.mf, self.SAM = generate_iso_card()    

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
        card_capabilities = self.mf.firstSFT + self.mf.secondSFT + \
                Iso7816OS.makeThirdSoftwareFunctionTable(extendedLe = extended_length)
        self.atr = Iso7816OS.makeATR(T=1, directConvention = True, TA1=0x13,
                histChars = chr(0x80) + chr(0x70 + len(card_capabilities)) +
                card_capabilities)

    def getATR(self):
        return self.atr
        
    @staticmethod
    def makeATR(**args): # {{{
        """Calculate Answer to Reset (ATR) and returns the bitstring.
        
        directConvention -- Bool. Whether to use direct convention or inverse convention.
        TAi, TBi, TCi    -- (optional) Value between 0 and 0xff. Interface Characters (for meaning see ISO 7816-3). Note that if no transmission protocol is given, it is automatically selected with T=max{j-1|TAj in args OR TBj in args OR TCj in args}.
        T                -- (optional) Value between 0 and 15. Transmission Protocol. Note that if T is set, TAi/TBi/TCi for i>T are omitted.
        histChars        -- (optional) Bitstring with 0 <= len(histChars) <= 15. Historical Characters T1 to T15 (for meaning see ISO 7816-4).
        
        T0, TDi and TCK are automatically calculated.
        """
        # first byte TS
        if args["directConvention"]:
            atr = "\x3b"
        else:
            atr = "\x3f"
            
        if args.has_key("T"):
            T = args["T"]
        else:
            T = 0

        # find maximum i of TAi/TBi/TCi in args
        maxTD = 0
        i = 15
        while i > 0:
            if args.has_key("TA" + str(i)) or args.has_key("TB" + str(i)) or args.has_key("TC" + str(i)):
                maxTD = i-1
                break
            i -= 1

        if maxTD == 0 and T > 0:
            maxTD = 2
                
        # insert TDi into args (TD0 is actually T0)
        for i in range(0, maxTD+1):
            if i == 0 and args.has_key("histChars"):
                args["TD0"] = len(args["histChars"])
            else:
                args["TD"+str(i)] = T

            if i < maxTD:
                args["TD"+str(i)] |= 1<<7

            if args.has_key("TA" + str(i+1)):
                args["TD"+str(i)] |= 1<<4
            if args.has_key("TB" + str(i+1)):
                args["TD"+str(i)] |= 1<<5
            if args.has_key("TC" + str(i+1)):
                args["TD"+str(i)] |= 1<<6
                
        # initialize checksum
        TCK = 0
        
        # add TDi, TAi, TBi and TCi to ATR (TD0 is actually T0)
        for i in range(0, maxTD+1):
            atr = atr + "%c" % args["TD" + str(i)]
            TCK ^= args["TD" + str(i)]
            for j in ["A", "B", "C"]:
                if args.has_key("T" + j + str(i+1)):
                    atr += "%c" % args["T" + j + str(i+1)]
                    # calculate checksum for all bytes from T0 to the end
                    TCK ^= args["T" + j + str(i+1)]
                    
        # add historical characters
        if args.has_key("histChars"):
            atr += args["histChars"]
            for i in range(0, len(args["histChars"])):
                TCK ^= ord( args["histChars"][i] )
        
        # checksum is omitted for T=0
        if T > 0:
            atr += "%c" % TCK
            
        return atr
    # }}}
    @staticmethod
    def makeThirdSoftwareFunctionTable(commandChainging=False,
            extendedLe=False, assignLogicalChannel=0, maximumChannels=0): # {{{ 
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
            if not (0<=assignLogicalChannel and assignLogicalChannel<=3):
                raise ValueError
            tsft |= assignLogicalChannel << 3
        if maximumChannels:
            if not (0<=maximumChannels and maximumChannels<=7):
                raise ValueError
            tsft |= maximumChannels
        return inttostring(tsft)
# }}} 

    def formatResult(self, le, data, sw, sm):
        self.lastCommandOffcut = data[le:]
        l = len(self.lastCommandOffcut)
        if l == 0:
            self.lastCommandSW = SW["NORMAL"]
        else:
            self.lastCommandSW = sw
            sw = SW["NORMAL_REST"] + min(0xff, l)

        result = data[:le]
        if sm:
            sw, result = self.SAM.protect_result(sw, result)

        return R_APDU(result, inttostring(sw)).render()

    def getResponse(self, p1, p2, data):
        if not (p1 == 0 and p2 == 0):
            raise SwError(SW["ERR_INCORRECTP1P2"])

        return self.lastCommandSW, self.lastCommandOffcut

    def execute(self, msg):
        def notImplemented(*argz, **args):
            raise SwError(SW["ERR_INSNOTSUPPORTED"])

        try:
            c = C_APDU(msg)
        except ValueError, e:
            logging.warning(str(e))
            return self.formatResult(0, "", SW["ERR_INCORRECTPARAMETERS"], False)

        #Handle Class Byte{{{
        class_byte = c.cla
        SM_STATUS = None
        logical_channel = 0
        command_chaining = 0
        header_authentication = 0
        
        #Ugly Hack for OpenSC-explorer
        if(class_byte == 0xb0):
            logging.debug("Open SC APDU")
            SM_STATUS = "No SM"
        
        #If Bit 8,7,6 == 0 then first industry values are used
        if (class_byte & 0xE0 == 0x00):
            #Bit 1 and 2 specify the logical channel
            logical_channel = class_byte & 0x03
            #Bit 3 and 4 specify secure messaging
            secure_messaging = class_byte >> 2
            secure_messaging &= 0x03
            if (secure_messaging == 0x00):
                SM_STATUS = "No SM"
            elif (secure_messaging == 0x01):
                SM_STATUS = "Propietary SM" # Not supported ?
            elif (secure_messaging == 0x02):
                SM_STATUS = "Standard SM" 
            elif (secure_messaging == 0x03):
                SM_STATUS = "Standard SM"
                header_authentication = 1
        #If Bit 8,7 == 01 then further industry values are used
        elif (class_byte & 0x0C == 0x0C):
            #Bit 1 to 4 specify logical channel. 4 is added, value range is from four to nineteen
            logical_channel = class_byte & 0x0f
            logical_channel += 4
            #Bit 6 indicates secure messaging
            secure_messaging = class_byte >> 5
            secure_messaging &= 0x01
            if (secure_messaging == 0x00):
                SM_STATUS = "No SM"            
            elif (secure_messaging == 0x01):
                SM_STATUS = "Standard SM"
        #In both cases Bit 5 specifiys command chaining
        command_chaining = class_byte >> 5
        command_chaining &= 0x01
        #}}}
        
        try:             
            if SM_STATUS == "Standard SM":
                c = self.SAM.parse_SM_CAPDU(c, header_authentication)
            elif SM_STATUS == "Propietary SM":
                raise SwError("ERR_SECMESSNOTSUPPORTED")
            sw, result = self.ins2handler.get(c.ins, notImplemented)(c.p1, c.p2, c.data)
            if SM_STATUS == "Standard SM":
                answer = self.formatResult(c.effective_Le, result, sw, True)
            else:
                answer = self.formatResult(c.effective_Le, result, sw, False)
        except SwError, e:
            logging.info(e.message)
            #traceback.print_exception(*sys.exc_info())
            sw = e.sw
            result = ""
            answer = self.formatResult(0, result, sw, False)

        return answer
# }}}
      
class CryptoflexOS(Iso7816OS): # {{{ 
    def __init__(self, mf, sam, ins2handler=None, maxle=MAX_SHORT_LE):
        Iso7816OS.__init__(self, mf, sam, ins2handler, maxle)
        self.atr = '\x3B\xE2\x00\x00\x40\x20\x49\x06'

    def execute(self, msg):
        def notImplemented(*argz, **args):
            raise SwError(SW["ERR_INSNOTSUPPORTED"])

        try:
            c = C_APDU(msg)
        except ValueError, e:
            logging.debug("Failed to parse APDU %s", msg)
            return self.formatResult(0, 0, "", SW["ERR_INCORRECTPARAMETERS"])

        try:
            sw, result = self.ins2handler.get(c.ins, notImplemented)(c.p1, c.p2, c.data)
            #print type(result)
        except SwError, e:
            logging.info(e.message)
            #traceback.print_exception(*sys.exc_info())
            sw = e.sw
            result = ""

        r = self.formatResult(c.ins, c.le, result, sw)
        return r

    def formatResult(self, ins, le, data, sw):
        if le == 0 and len(data):
            # cryptoflex does not inpterpret le==0 as maxle
            self.lastCommandSW = sw
            self.lastCommandOffcut = data
            r = R_APDU(inttostring(SW["ERR_WRONGLENGTH"] +\
                    min(0xff, len(data)))).render()
        else:
            if ins == 0xa4 and len(data):
                # get response should be followed by select file
                self.lastCommandSW = sw
                self.lastCommandOffcut = data
                r = R_APDU(inttostring(SW["NORMAL_REST"] +\
                    min(0xff, len(data)))).render()
            else:
                r = Iso7816OS.formatResult(self, le, data, sw, False)

        return r
# }}}

class RelayOS(SmartcardOS): # {{{

    def __init__(self, readernum):
        readers = smartcard.System.listReaders()
        if len(readers) <= readernum:
            logging.error("Invalid number of reader '%u' (only %u available)"
                          % (readernum, len(readers)))
            sys.exit()
        # XXX this is a workaround, see on sourceforge bug #3083254
        # should better use
        # self.reader = smartcard.System.readers()[readernum]
        self.reader = readers[readernum]
        # set exit to True if you want to terminate relaying
        self.exit = False
        try:
            self.session = smartcard.Session(self.reader)
        except smartcard.Exceptions.CardConnectionException, e:
            logging.error("Error connecting to card: %s", e.message)
            sys.exit()

        logging.info("Connected to card in '%s'", self.reader)

        atexit.register(self.cleanup)

    def cleanup(self):
        try:
            self.session.close()
        except smartcard.Exceptions.CardConnectionException, e:
            logging.warning("Error disconnecting from card: %s", e.message)

    def getATR(self):
        try:
            atr = self.session.getATR()
        except smartcard.Exceptions.CardConnectionException, e:
            try:
                self.session.close()
                self.session = smartcard.Session(self.reader)
                atr = self.session.getATR()
            except smartcard.Exceptions.CardConnectionException, e:
                logging.error("Error getting ATR: %s", e.message)
                sys.exit()

        return "".join([chr(b) for b in atr])
        
    def powerUp(self):
        try:
            self.session.getATR()
        except smartcard.Exceptions.CardConnectionException, e:
            try:
                self.session = smartcard.Session(self.reader)
            except smartcard.Exceptions.CardConnectionException, e:
                logging.error("Error connecting to card: %s", e.message)
                sys.exit()

    def powerDown(self):
        try:
            self.session.close()
        except smartcard.Exceptions.CardConnectionException, e:
            logging.error("Error disconnecting from card: %s", str(e))
            sys.exit()

    def execute(self, msg):
        #apdu = [].append(ord(b) for b in msg)
        apdu = []
        for b in msg:
            apdu.append(ord(b))

        try:
            rapdu, sw1, sw2 = self.session.sendCommandAPDU(apdu)
        except smartcard.Exceptions.CardConnectionException, e:
            logging.error("Error transmitting APDU: %s", str(e))
            sys.exit()

        # XXX this is a workaround, see on sourceforge bug #3083586
        # should better use
        # rapdu = rapdu + [sw1, sw2]
        if rapdu[-2:] == [sw1, sw2]:
            pass
        else:
            rapdu = rapdu + [sw1, sw2]

        return "".join([chr(b) for b in rapdu])
# }}}
      


# sizeof(int) taken from asizof-package {{{
_Csizeof_short = len(struct.pack('h', 0))
# }}}

VPCD_CTRL_LEN 	= 1

VPCD_CTRL_OFF   = 0
VPCD_CTRL_ON    = 1
VPCD_CTRL_RESET = 2
VPCD_CTRL_ATR	= 4

class VirtualICC(object): # {{{
    """
    This class is responsible for maintaining the communication of the virtual
    PCD and the emulated smartcard. vpicc and vpcd communicate via a socket.
    The vpcd sends command APDUs (which it receives from an application) to the
    vicc. The vicc passes these CAPDUs on to an emulated smartcard, which
    produces a response APDU. This RAPDU is then passed back by the vicc to
    the vpcd, which forwards it to the application.
    """ 
    
    def __init__(self, filename, type, host, port, lenlen=3, readernum=None):
        from os.path import exists
        
        logging.basicConfig(level = logging.INFO, 
                            format = "%(asctime)s  [%(levelname)s] %(message)s", 
                            datefmt = "%d.%m.%Y %H:%M:%S") 
        
        self.filename = None
        self.cardGenerator = CardGenerator.CardGenerator(type)
        
        #If a filename is specified, try to load the card from disk      
        if filename != None:
            self.filename = filename
            if exists(filename):
                self.cardGenerator.loadCard(self.filename)
            else:
                logging.info("Creating new card which will be saved in %s.",
                              self.filename)
        
        MF, SAM = self.cardGenerator.getCard()
        
        #Generate an OS object of the correct type
        if type == "iso7816" or type == "ePass":
            self.os = Iso7816OS(MF, SAM)
        elif type == "cryptoflex":
            self.os = CryptoflexOS(MF, SAM)
        elif type == "relay":
            self.os = RelayOS(readernum)
        else:
            logging.warning("Unknown cardtype %s. Will use standard type (ISO 7816)",
                            type)
            type = "iso7816"
            self.os = Iso7816OS(MF, SAM)
        self.type = type
            
        #Connect to the VPCD
        try:
            self.sock = self.connectToPort(host, port)
            self.sock.settimeout(None)
        except socket.error, e:
            logging.error("Failed to open socket: %s", str(e))
            logging.error("Is pcscd running at %s? Is vpcd loaded? Is a firewall blocking port %u?" % (host, port))
            sys.exit()
                       
        self.lenlen = lenlen

        logging.info("Connected to virtual PCD at %s:%u" % (host, port))

        signal.signal(signal.SIGINT, self.signalHandler)
        atexit.register(self.stop)
    
    def signalHandler(self, sig=None, frame=None):
        """Basically this signal handler just surpresses a traceback from being
           printed when the user presses crtl-c"""
        sys.exit()

    @staticmethod
    def connectToPort(host, port):
        """
        Open a connection to a given host on a given port.
        """
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        return sock

    def __sendToVPICC(self, msg):
        """ Send a message to the vpcd """
        #size = inttostring(len(msg), self.lenlen)
        self.sock.send(struct.pack('!H', len(msg)) + msg)

    def __recvFromVPICC(self):
        """ Receive a message from the vpcd """
        # receive message size
        sizestr = self.sock.recv(_Csizeof_short)
        if len(sizestr) == 0:
            logging.info("Virtual PCD shut down")
            sys.exit()
        size = struct.unpack('!H', sizestr)[0]

        # receive and return message
        if size:
            msg = self.sock.recv(size)
            if len(msg) == 0:
                logging.info("Virtual PCD shut down")
        else:
            msg = None

        return size, msg

    def run(self):
        """
        Main loop of the vpicc. Receives command APDUs via a socket from the
        vpcd, dispatches them to the emulated smartcard and sends the resulting
        respsonse APDU back to the vpcd.
        """
        while True :
            (size, msg) = self.__recvFromVPICC()
            if not size:
                logging.warning("Error in communication protocol (missing size parameter)")
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
                    logging.warning("Expected APDU of %u bytes, but received only %u" % (size, len(msg)))

                logging.info("APDU (%d Bytes):\n%s" % (len(msg), hexdump(msg, short=True)))
                answer = self.os.execute(msg)
                logging.info("RESP (%d Bytes):\n%s\n" % (len(answer), hexdump(answer, short=True)))
                self.__sendToVPICC(answer)

    def stop(self):
        self.sock.close()
        if self.filename != None:
            self.cardGenerator.setCard(self.os.mf, self.os.SAM)
            self.cardGenerator.saveCard(self.filename)
# }}} 
