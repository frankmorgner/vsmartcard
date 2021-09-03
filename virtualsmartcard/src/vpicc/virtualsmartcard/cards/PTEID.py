#
# Copyright (C) 2021 João Paulo Barraca
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

from virtualsmartcard.SmartcardSAM import SAM
from virtualsmartcard.SWutils import SwError, SW
from virtualsmartcard.SEutils import ControlReferenceTemplate, \
    Security_Environment
from virtualsmartcard.utils import C_APDU, hexdump
from virtualsmartcard.VirtualSmartcard import Iso7816OS
from virtualsmartcard.SmartcardFilesystem import MF, DF, EF
from virtualsmartcard.ConstantDefinitions import MAX_SHORT_LE
import virtualsmartcard.CryptoUtils as vsCrypto
from virtualsmartcard.utils import inttostring, stringtoint, C_APDU, R_APDU
import logging
from binascii import hexlify, b2a_base64, a2b_base64
import sys
from Crypto.Cipher import PKCS1_v1_5
from Crypto.PublicKey import RSA
from Crypto.Hash import SHA
import json

logger = logging.getLogger('pteid')
logger.setLevel(logging.DEBUG)

class PTEIDOS(Iso7816OS):
    def __init__(self, mf, sam, ins2handler=None, maxle=MAX_SHORT_LE):
        Iso7816OS.__init__(self, mf, sam, ins2handler, maxle)
        self.atr = '\x3B\x7D\x95\x00\x00\x80\x31\x80\x65\xB0\x83\x11\x00\xC8\x83\x00\x90\x00'

    def execute(self, msg):
        def notImplemented(*argz, **args):
            raise SwError(SW["ERR_INSNOTSUPPORTED"])

        logger.info("Command APDU (%d bytes):\n  %s", len(msg),
                     hexdump(msg, indent=2))

        try:
            c = C_APDU(msg)
        except ValueError as e:
            logger.exception(f"Failed to parse {e} APDU {msg}")
            return self.formatResult(False, 0, 0, "",
                                     SW["ERR_INCORRECTPARAMETERS"])

        try:
            if c.ins == 0x80:
                logger.info("Handle 0x80")
                sw, result = self.sam.handle_0x80(c.p1, c.p2, c.data)
            else:
                sw, result = self.ins2handler.get(c.ins, notImplemented)(c.p1,
                                                                         c.p2,
                                                                         c.data)
        except SwError as e:
            #logger.error(self.ins2handler.get(c.ins, None))
            logger.exception("SWERROR")
            sw = e.sw
            result = ""
        except Exception as e:
            logger.exception(f"ERROR: {e}")

        r = self.formatResult(c.ins, c.p1, c.p2, c.le, result, sw)
        return r

    def formatResult(self, ins, p1, p2, le, data, sw):
        logger.debug(
            f"FormatResult: ins={hex(ins)} le={le} length={len(data)} sw={hex(sw)}")
        
        if ins == 0x2a and p1 == 0x9e and p2 == 0x9a and le != len(data):
            r = R_APDU(inttostring(0x6C00 + len(data))).render()
        else:
            if ins == 0xb0 and le == 0 or ins == 0xa4:
                le = min(255, len(data))

            if ins == 0xa4 and len(data):
                self.lastCommandSW = sw
                self.lastCommandOffcut = data
                r = R_APDU(inttostring(SW["NORMAL_REST"] +
                                   min(0xff, len(data) ))).render()
            else:
                r = Iso7816OS.formatResult(self, Iso7816OS.seekable(ins), le,
                                       data, sw, False)
        return r



class PTEID_SE(Security_Environment):

    def __init__(self, MF, SAM):
        Security_Environment.__init__(self, MF, SAM)
        logger.debug("Using PTEID SE")
        self.at.algorithm = 'SHA'
        self.data_to_sign = b''
        self.signature = b''

        logger.debug(f"AT: {self.at.algorithm}")

    def compute_digital_signature(self, p1, p2, data):

        """
        Compute a digital signature for the given data.
        Algorithm and key are specified in the current SE
        """
        if self.data_to_sign == b'':
            return self.signature

        logging.info(f"Compute digital signature {hex(p1)} {hex(p2)} {len(self.data_to_sign)} {hexlify(self.data_to_sign)}")

        if p1 != 0x9E or p2 not in (0x9A, 0xAC, 0xBC):
            raise SwError(SW["ERR_INCORRECTP1P2"])

        if self.dst.key is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        if self.data_to_sign is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        to_sign = b''
        if p2 == 0x9A:  # Data to be signed
            to_sign = self.data_to_sign
        elif p2 == 0xAC:  # Data objects, sign values
            to_sign = b''
            structure = unpack(data)
            for tag, length, value in structure:
                to_sign += value
        elif p2 == 0xBC:  # Data objects to be signed
            logging.warning("Data objects to be signed: 0xBC")
            pass
        
        logging.debug(f"Actual data signed: {hexlify(to_sign)}")
        signature = self.dst.key.sign(to_sign, "")
        return signature

    def hash(self, p1, p2, data):
        """
        Hash the given data using the algorithm specified by the current
        Security environment.

        :return: raw data (no TLV coding).
        """
        logging.info(f"Compute Hash {hex(p1)} {hex(p2)} {data[2:]}")
        hash = super().hash(p1, p2, data[2:])

        self.data_to_sign = hash

        return hash

class PTEID_SAM(SAM):
    def __init__(self, mf=None, private_key=None):
        SAM.__init__(self, None, None, mf, default_se=PTEID_SE)
        self.current_SE = self.default_se(self.mf, self)
        self.PIN = b'1111'

        self.current_SE.ht.algorithm = "SHA"
        self.current_SE.algorithm = "AES-CBC"


        self.current_SE.dst.key = RSA.importKey(private_key)

    def handle_0x80(self, p1, p2, data):
        logger.debug(f"Handle 0x80 {hex(p1)} {hex(p2)} {hex(data)}")
        sys.exit(-1)
        return 0x9000, b''

    def parse_SE_config(self, config):
        r = 0x9000
        logger.debug(type(config))
        logger.debug(f"Parse SE config {hexlify(config)}")

        try:
            ControlReferenceTemplate.parse_SE_config(self, config)
        except SwError as e:
            logger.exception("e")

        return r, b''

    def verify(self, p1, p2, PIN):
        logging.debug("Received PIN: %s", PIN.strip())
        PIN = PIN.replace(b"\0", b"").strip()  # Strip NULL characters
        PIN = PIN.replace(b"\xFF", b"")  # Strip \xFF characters
        logging.debug("PIN to use: %s", PIN)

        if len(PIN) == 0:
            raise SwError(SW["WARN_NOINFO63"])

        return super().verify(p1, p2, PIN)

class PTEID_MF(MF):  # {{{
    def getDataPlain(self, p1, p2, data):

        logger.info(
            f"GetData Plain {hex(p1)} {hex(p2)} {hexlify(data)}")

        tag = (p1 << 8) + p2
        if tag == 0xDF03:
            return 0x6E00, b''

        sys.exit(0)
        return 0x9000, b''

    def readBinaryPlain(self, p1, p2, data):
        logger.debug(f"Read Binary {hex(p1)} {hex(p2)}")
        if p2 != 0:
            p1 = 0
            p2 = 0
        try:
            sw, result = super().readBinaryPlain(p1, p2, data)
            return 0x9000, result
        except Exception as e:
            logger.exception(f"{e}")


    def selectFile(self, p1, p2, data):
        # return super().selectFile(p1, p2, data)
        """
        Function for instruction 0xa4. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string. Returns the status bytes as two
        byte long integer and the response data as binary string.
        """
        logger.debug(f"SelectFile: fid=0x{hexlify(data).decode('latin')} p2={p2}")

        P2_FCI = p2 & (3 << 2) == 0
        P2_FCP = p2 & 4 != 0
        P2_FMD = p2 & 8 != 0
        P2_NONE = 3 << 2


        logger.debug(f"FCI: {P2_FCI}, FCP: {P2_FCP} FMD:{P2_FMD}")

        # Will fail with exception if File Not Found
        file = self._selectFile(p1, p2, data)
        self.current = file

        if isinstance(file, EF):
            logger.debug("IS EF")
            fci = self.get_fci(file)
            return 0x9000, fci

        elif isinstance(file, DF):
            # Search by Name
            if p1 == 0x04:
                return 0x9000, b''

            logger.debug("IS DF")
            fci = self.get_fci(file)
            return 0x9000, fci

    def get_fci(self, file):
        try:
            if isinstance(file, EF):
                fcid = b'\x6F\x15' + \
                    b'\x81\x02' + len(file.data).to_bytes(2, byteorder='big') + \
                    b'\x82\x01\x01' +  \
                    b'\x8a\x01\x05' +\
                    b'\x83\x02' + file.fid.to_bytes(2, byteorder='big')+ \
                    file.extra_fci_data
            else:
                fcid = b'\x6F\x14' + \
                    b'\x83\x02' + file.fid.to_bytes(2, byteorder='big') + \
                    file.extra_fci_data
                print(file.extra_fci_data)
                name = getattr(file, 'dfname')
                if len(name) > 0:
                    fcid = fcid + b'\x84' + len(name).to_bytes(1, byteorder='big') + name

            return fcid
        except Exception as e:
            logger.exception(f"get fci: {e}")
            return b''

# }}}
