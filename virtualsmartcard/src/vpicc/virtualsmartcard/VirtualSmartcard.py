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
from virtualsmartcard.CardGenerator import CardGenerator

import socket, struct, sys, signal, atexit, smartcard, logging


class SmartcardOS(object): 
    """Base class for a smart card OS"""

    mf  = make_property("mf",  "master file")
    SAM = make_property("SAM", "secure access module")

    def getATR(self):
        """Returns the ATR of the card as string of characters"""
        return ""
        
    def powerUp(self):
        """Powers up the card"""
        self.mf.current = self.mf
        pass

    def powerDown(self):
        """Powers down the card"""
        pass

    def reset(self):
        """Performs a warm reset of the card (no power down)"""
        self.mf.current = self.mf
        pass

    def execute(self, msg):
        """Returns response to the given APDU as string of characters
        
        :param msg: the APDU as string of characters
        """
        return ""


class Iso7816OS(SmartcardOS):  
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
                                    Historical Characters T1 to T15 (for meaning 
                                    see ISO 7816-4).
        
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
    
    @staticmethod
    def makeThirdSoftwareFunctionTable(commandChainging=False,
            extendedLe=False, assignLogicalChannel=0, maximumChannels=0):  
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

        result = data[:le]
        if sm:
            sw, result = self.SAM.protect_result(sw, result)

        return R_APDU(result, inttostring(sw)).render()

    @staticmethod
    def seekable(ins):
        if ins in [0xb0, 0xb1, 0xd0, 0xd1, 0xd6, 0xd7, 0xa0, 0xa1, 0xb2, 0xb3, 0xdc, 0xdd ]:
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
            return self.formatResult(False, 0, "", SW["ERR_INCORRECTPARAMETERS"], False)

        logging.info("Parsed APDU:\n%s", str(c))
        
        #Handle Class Byte
        #{{{
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
                SM_STATUS = "Proprietary SM" # Not supported ?
            elif (secure_messaging == 0x02):
                SM_STATUS = "Standard SM" 
            elif (secure_messaging == 0x03):
                SM_STATUS = "Standard SM"
                header_authentication = 1
        #If Bit 8,7 == 01 then further industry values are used
        elif (class_byte & 0x0C == 0x0C):
            #Bit 1 to 4 specify logical channel. 4 is added, value range is from
            #four to nineteen
            logical_channel = class_byte & 0x0f
            logical_channel += 4
            #Bit 6 indicates secure messaging
            secure_messaging = class_byte >> 6
            if (secure_messaging == 0x00):
                SM_STATUS = "No SM" 
            elif (secure_messaging == 0x01):
                SM_STATUS = "Standard SM"
            else:
                # Bit 8 is set to 1, which is not specified by ISO 7816-4
                SM_STATUS = "Proprietary SM"
        #In both cases Bit 5 specifies command chaining
        command_chaining = class_byte >> 5
        command_chaining &= 0x01
        #}}}
        
        sm = False
        try:             
            if SM_STATUS == "Standard SM" or SM_STATUS == "Proprietary SM":
                c = self.SAM.parse_SM_CAPDU(c, header_authentication)
                logging.info("Decrypted APDU:\n%s", str(c))
                sm = True
            sw, result = self.ins2handler.get(c.ins, notImplemented)(c.p1, c.p2, c.data)
            answer = self.formatResult(Iso7816OS.seekable(c.ins), c.effective_Le, result, sw, sm)
        except SwError as e:
            logging.info(e.message)
            import traceback
            traceback.print_exception(*sys.exc_info())
            sw = e.sw
            result = ""
            answer = self.formatResult(False, 0, result, sw, sm)

        return answer


class CryptoflexOS(Iso7816OS):  
    def __init__(self, mf, sam, ins2handler=None, maxle=MAX_SHORT_LE):
        Iso7816OS.__init__(self, mf, sam, ins2handler, maxle)
        self.atr = '\x3B\xE2\x00\x00\x40\x20\x49\x06'

    def execute(self, msg):
        def notImplemented(*argz, **args):
            raise SwError(SW["ERR_INSNOTSUPPORTED"])

        try:
            c = C_APDU(msg)
        except ValueError as e:
            logging.debug("Failed to parse APDU %s", msg)
            return self.formatResult(False, 0, 0, "", SW["ERR_INCORRECTPARAMETERS"])

        try:
            sw, result = self.ins2handler.get(c.ins, notImplemented)(c.p1, c.p2, c.data)
            #print type(result)
        except SwError as e:
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
                r = Iso7816OS.formatResult(self, Iso7816OS.seekable(ins), le, data, sw, False)

        return r


class RelayOS(SmartcardOS):
    """
    This class implements relaying of a (physical) smartcard. The RelayOS
    forwards the command APDUs received from the vpcd to the real smartcard via
    an actual smart card reader and sends the responses back to the vpcd.
    This class can be used to implement relay or MitM attacks.
    """
    def __init__(self, readernum):
        """
        Initialize the connection to the (physical) smart card via a given reader
        """

        # See which readers are available
        readers = smartcard.System.listReaders()
        if len(readers) <= readernum:
            logging.error("Invalid number of reader '%u' (only %u available)",
                          readernum, len(readers))
            sys.exit()

        # Connect to the reader and its card
        # XXX this is a workaround, see on sourceforge bug #3083254
        # should better use
        # self.reader = smartcard.System.readers()[readernum]
        self.reader = readers[readernum]
        try:
            self.session = smartcard.Session(self.reader)
        except smartcard.Exceptions.CardConnectionException as e:
            logging.error("Error connecting to card: %s", e.message)
            sys.exit()

        logging.info("Connected to card in '%s'", self.reader)

        atexit.register(self.cleanup)

    def cleanup(self):
        """
        Close the connection to the physical card
        """
        try:
            self.session.close()
        except smartcard.Exceptions.CardConnectionException as e:
            logging.warning("Error disconnecting from card: %s", e.message)

    def getATR(self):
        # when powerDown has been called, fetching the ATR will throw an error.
        # In this case we must try to reconnect (and then get the ATR).
        try:
            atr = self.session.getATR()
        except smartcard.Exceptions.CardConnectionException as e:
            try:
                # Try to reconnect to the card
                self.session.close()
                self.session = smartcard.Session(self.reader)
                atr = self.session.getATR()
            except smartcard.Exceptions.CardConnectionException as e:
                logging.error("Error getting ATR: %s", e.message)
                sys.exit()

        return "".join([chr(b) for b in atr])
        
    def powerUp(self):
        # When powerUp is called multiple times the session is valid (and the
        # card is implicitly powered) we can check for an ATR. But when
        # powerDown has been called, the session gets lost. In this case we
        # must try to reconnect (and power the card).
        try:
            self.session.getATR()
        except smartcard.Exceptions.CardConnectionException as e:
            try:
                self.session = smartcard.Session(self.reader)
            except smartcard.Exceptions.CardConnectionException as e:
                logging.error("Error connecting to card: %s", e.message)
                sys.exit()

    def powerDown(self):
        # There is no power down in the session context so we simply
        # disconnect, which should implicitly power down the card.
        try:
            self.session.close()
        except smartcard.Exceptions.CardConnectionException as e:
            logging.error("Error disconnecting from card: %s", str(e))
            sys.exit()

    def execute(self, msg):
        # sendCommandAPDU() expects a list of APDU bytes
        apdu = map(ord, msg)

        try:
            rapdu, sw1, sw2 = self.session.sendCommandAPDU(apdu)
        except smartcard.Exceptions.CardConnectionException as e:
            logging.error("Error transmitting APDU: %s", str(e))
            sys.exit()

        # XXX this is a workaround, see on sourceforge bug #3083586
        # should better use
        # rapdu = rapdu + [sw1, sw2]
        if rapdu[-2:] == [sw1, sw2]:
            pass
        else:
            rapdu = rapdu + [sw1, sw2]

        # return the response APDU as string
        return "".join([chr(b) for b in rapdu])


class NPAOS(Iso7816OS):
    def __init__(self, mf, sam, ins2handler=None, maxle=MAX_EXTENDED_LE, ef_cardsecurity=None, ef_cardaccess=None, ca_key=None, cvca=None, disable_checks=False):
        Iso7816OS.__init__(self, mf, sam, ins2handler, maxle)
        self.ins2handler[0x86] = self.SAM.general_authenticate
        self.ins2handler[0x2c] = self.SAM.reset_retry_counter

        # different ATR (Answer To Reset) values depending on used Chip version
        # It's just a playground, because in past one of all those eID clients did not recognize the card correctly with newest ATR values
        self.atr = '\x3B\x8A\x80\x01\x80\x31\xF8\x73\xF7\x41\xE0\x82\x90\x00\x75'
        #self.atr = '\x3B\x8A\x80\x01\x80\x31\xB8\x73\x84\x01\xE0\x82\x90\x00\x06'
        #self.atr = '\x3B\x88\x80\x01\x00\x00\x00\x00\x00\x00\x00\x00\x09'
        #self.atr = '\x3B\x87\x80\x01\x80\x31\xB8\x73\x84\x01\xE0\x19'

        self.SAM.current_SE.disable_checks = disable_checks
        if ef_cardsecurity:
            ef = self.mf.select('fid', 0x011d)
            ef.data = ef_cardsecurity
        if ef_cardaccess:
            ef = self.mf.select('fid', 0x011c)
            ef.data = ef_cardaccess
        if cvca:
            self.SAM.current_SE.cvca = cvca
        if ca_key:
            self.SAM.current_SE.ca_key = ca_key


    def formatResult(self, seekable, le, data, sw, sm):
        if seekable:
            # when le = 0 then we want to have 0x9000. here we only have the
            # effective le, which is either MAX_EXTENDED_LE or MAX_SHORT_LE,
            # depending on the APDU. Note that the following distinguisher has
            # one false positive
            if le > len(data) and le != MAX_EXTENDED_LE and le != MAX_SHORT_LE:
                sw = SW["WARN_EOFBEFORENEREAD"]

        result = data[:le]
        if sm:
            try:
                sw, result = self.SAM.protect_result(sw, result)
            except SwError as e:
                logging.info(e.message)
                import traceback
                traceback.print_exception(*sys.exc_info())
                sw = e.sw
                result = ""
                answer = self.formatResult(False, 0, result, sw, False)

        return R_APDU(result, inttostring(sw)).render()


class HandlerTestOS(SmartcardOS):
    """
    This class implements the commands used for the PC/SC-lite  smart  card
    reader driver tester. See http://pcsclite.alioth.debian.org/pcsclite.html
    and handler_test(1).
    """
    def __init__(self):
        lastCommandOffcut = ''

    def getATR(self):
        return '\x3B\xD6\x18\x00\x80\xB1\x80\x6D\x1F\x03\x80\x51\x00\x61\x10\x30\x9E'
        
    def powerUp(self):
        pass

    def reset(self):
        pass

    def __output_from_le(self, msg):
        le = (ord(msg[2])<<8)+ord(msg[3])
        return ''.join([chr(num&0xff) for num in xrange(le)])

    def execute(self, msg):
        ok = '\x90\x00'
        error = '\x6d\x00'
        if msg == '\x00\xA4\x04\x00\x06\xA0\x00\x00\x00\x18\x50' or msg == '\x00\xA4\x04\x00\x06\xA0\x00\x00\x00\x18\xFF':
            logging.info('Select applet')
            return ok
        elif msg.startswith('\x80\x38\x00'):
            logging.info('Time Request')
            return ok
        elif msg == '\x80\x30\x00\x00':
            logging.info('Case 1, APDU')
            return ok
        elif msg == '\x80\x30\x00\x00\x00':
            logging.info('Case 1, TPDU')
            return ok
        elif msg.startswith('\x80\x32\x00\x00'):
            logging.info('Case 3')
            return ok
        elif msg.startswith('\x80\x34'):
            logging.info('Case 2')
            return self.__output_from_le(msg) + ok
        elif msg.startswith('\x80\x36'):
            if len(msg) == 5+ord(msg[4]):
                logging.info('Case 4, TPDU')
                self.lastCommandOffcut = self.__output_from_le(msg)
                if len(self.lastCommandOffcut) > 0xFF:
                    return '\x61\x00'
                return '' + chr(len(self.lastCommandOffcut)&0xFF)
            elif len(msg) == 6+ord(msg[4]):
                logging.info('Case 4, APDU')
                return self.__output_from_le(msg) + ok
            else:
                return error
        elif msg.startswith('\x80\xC0\x00\x00'):
            logging.info('Get response')
            out = self.lastCommandOffcut[:ord(msg[4])]
            self.lastCommandOffcut = self.lastCommandOffcut[ord(msg[4]):]
            return out + ok
        else:
            return error



# sizeof(int) taken from asizof-package {{{
_Csizeof_short = len(struct.pack('h', 0))
# }}}


VPCD_CTRL_LEN 	= 1

VPCD_CTRL_OFF   = 0
VPCD_CTRL_ON    = 1
VPCD_CTRL_RESET = 2
VPCD_CTRL_ATR	= 4

class VirtualICC(object): 
    """
    This class is responsible for maintaining the communication of the virtual
    PCD and the emulated smartcard. vpicc and vpcd communicate via a socket.
    The vpcd sends command APDUs (which it receives from an application) to the
    vicc. The vicc passes these CAPDUs on to an emulated smartcard, which
    produces a response APDU. This RAPDU is then passed back by the vicc to
    the vpcd, which forwards it to the application.
    """ 
    
    def __init__(self, filename, datasetfile, card_type, host, port, readernum=None, ef_cardsecurity=None, ef_cardaccess=None, ca_key=None, cvca=None, disable_checks=False, logginglevel=logging.INFO):
        from os.path import exists
        
        logging.basicConfig(level = logginglevel, 
                            format = "%(asctime)s  [%(levelname)s] %(message)s", 
                            datefmt = "%d.%m.%Y %H:%M:%S") 
        
        self.filename = None
        self.cardGenerator = CardGenerator(card_type)
        
        #If a filename is specified, try to load the card from disk      
        if filename != None:
            self.filename = filename
            if exists(filename):
                self.cardGenerator.loadCard(self.filename)
            else:
                logging.info("Creating new card which will be saved in %s.",
                              self.filename)

        #If a dataset file is specified, read the card's data groups from disk
        if datasetfile != None:
            if exists(datasetfile):
                logging.info("Reading Data Groups from file %s.",
                        datasetfile)
                self.cardGenerator.readDatagroups(datasetfile)

        MF, SAM = self.cardGenerator.getCard()
        
        #Generate an OS object of the correct card_type
        if card_type == "iso7816" or card_type == "ePass":
            self.os = Iso7816OS(MF, SAM)
        elif card_type == "nPA":
            self.os = NPAOS(MF, SAM, ef_cardsecurity=ef_cardsecurity, ef_cardaccess=ef_cardaccess, ca_key=ca_key, cvca=cvca, disable_checks=disable_checks)
        elif card_type == "cryptoflex":
            self.os = CryptoflexOS(MF, SAM)
        elif card_type == "relay":
            self.os = RelayOS(readernum)
        elif card_type == "handler_test":
            self.os = HandlerTestOS()
        else:
            logging.warning("Unknown cardtype %s. Will use standard card_type (ISO 7816)",
                            card_type)
            card_type = "iso7816"
            self.os = Iso7816OS(MF, SAM)
        self.type = card_type
            
        #Connect to the VPCD
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
                logging.error("Is pcscd running at %s? Is vpcd loaded? Is a firewall blocking port %u?",
                              host, port)
                sys.exit()
        else:
            # use reversed connection mode
            try:
                (self.sock, self.server_sock, host) = self.openPort(port)
                self.sock.settimeout(None)
            except socket.error as e:
                logging.error("Failed to open socket: %s", str(e))
                logging.error("Is pcscd running? Is vpcd loaded and in reversed connection mode? Is a firewall blocking port %u?",
                              port)
                sys.exit()

        logging.info("Connected to virtual PCD at %s:%u", host, port)

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
        sizestr = self.sock.recv(_Csizeof_short)
        if len(sizestr) == 0:
            logging.info("Virtual PCD shut down")
            raise socket.error
        size = struct.unpack('!H', sizestr)[0]

        # receive and return message
        if size:
            msg = self.sock.recv(size)
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
        while True :
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
        if self.filename != None:
            self.cardGenerator.setCard(self.os.mf, self.os.SAM)
            self.cardGenerator.saveCard(self.filename)
 
