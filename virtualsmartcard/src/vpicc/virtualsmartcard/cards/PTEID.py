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
import json

logger = logging.getLogger('pteid')
logger.setLevel(logging.DEBUG)

from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.serialization import load_der_private_key
from cryptography.hazmat.backends import default_backend

class PTEIDOS(Iso7816OS):
    def __init__(self, mf, sam, ins2handler=None, maxle=MAX_SHORT_LE):
        Iso7816OS.__init__(self, mf, sam, ins2handler, maxle)
        self.atr = '\x3B\xFF\x96\x00\x00\x81\x31\xFE\x43\x80\x31\x80\x65\xB0\x85\x04\x01\x20\x12\x0F\xFF\x82\x90\x00\xD0'

    def execute(self, msg):
        def notImplemented(*argz, **args):
            raise SwError(SW["ERR_INSNOTSUPPORTED"])

        logger.debug("Command APDU (%d bytes):\n  %s", len(msg),
                     hexdump(msg, indent=2))
        
        try:
            c = C_APDU(msg)
        except ValueError as e:
            logger.exception(f"Failed to parse {e} APDU {msg}")
            return self.formatResult(False, 0, 0, "",
                                     SW["ERR_INCORRECTPARAMETERS"])
        
        result = ''
        sw = 0x900

        try:
            logger.debug(f"Handle {hex(c.ins)}")
            if c.ins == 0x80:
                sw, result = self.sam.handle_0x80(c.p1, c.p2, c.data)
            else:
                sw, result = self.ins2handler.get(c.ins, notImplemented)(c.p1,
                                                                         c.p2,
                                                                         c.data)
        except SwError as e:
            #logger.error(self.ins2handler.get(c.ins, None))
            logger.exception("SWERROR")
            sw = e.sw
        except Exception as e:
            logger.exception(f"ERROR: {e}")
        if isinstance(result, str):
            result = result.encode()

        logger.debug(f"Result: {hexlify(result)} {hex(sw)}")

        r = self.formatResult(c.ins, c.p1, c.p2, c.le, result, sw)
        return r

    def formatResult(self, ins, p1, p2, le, data, sw):
        logger.debug(
            f"FormatResult: ins={hex(ins)} p1={hex(p1)} p2={hex(p2)} le={le} length={len(data)} sw={hex(sw)}")
        
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

        logger.debug(f"Compute digital signature p1={hex(p1)} p2={hex(p2)} dsl={len(self.data_to_sign)} ds={hexlify(self.data_to_sign)} d={data}")

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
            logger.warning("Data objects to be signed: 0xBC")
        
        logger.debug(f"Actual data signed: {hexlify(to_sign)}")
        self.signature = bytes(self.dst.key.sign(
            to_sign,
            padding.PKCS1v15(),
            hashes.SHA1()
        ))

        logger.debug(f"Signature: {self.signature}")
        return self.signature


    def hash(self, p1, p2, data):
        """
        Hash the given data using the algorithm specified by the current
        Security environment.

        :return: raw data (no TLV coding).
        """
        logger.debug("Compute Hash {hex(p1)} {hex(p2)} {hexlify(data[17:])}")
        
        ## OpenSC driver will add data to beginning. Must remove it.
        hash_data = data[-20:] #super().hash(p1, p2, data[17:])
        logger.debug(f"Hash_data: {hexlify(hash_data)}")

        self.data_to_sign = hash_data

        return hash_data

class PTEID_SAM(SAM):
    def __init__(self, mf=None, private_key=None):
        SAM.__init__(self, None, None, mf, default_se=PTEID_SE)
        self.current_SE = self.default_se(self.mf, self)
        self.PIN = b'1111'

        self.current_SE.ht.algorithm = "SHA"
        self.current_SE.algorithm = "AES-CBC"

        self.current_SE.dst.key = load_der_private_key(private_key, password=None, backend=default_backend())

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
        PIN = PIN.replace(b"\0", b"").strip()  # Strip NULL characters
        PIN = PIN.replace(b"\xFF", b"")  # Strip \xFF characters
        logger.debug("PIN to use: %s", PIN)

        if len(PIN) == 0:
            raise SwError(SW["WARN_NOINFO63"])

        return super().verify(p1, p2, PIN)

class PTEID_MF(MF):  # {{{
    def getDataPlain(self, p1, p2, data):

        logger.debug(
            f"GetData Plain {hex(p1)} {hex(p2)} {hexlify(data)}")

        tag = (p1 << 8) + p2
        if tag == 0xDF03:
            return 0x6E00, b''

        sys.exit(0)
        return 0x9000, b''

    def readBinaryPlain(self, p1, p2, data):
        logger.debug(f"Read Binary P1={hex(p1)} P2={hex(p2)} {data}")
        ef, offsets, datalist = self.dataUnitsDecodePlain(p1, p2, data)
        logger.debug(f"EF={ef} offsets={offsets} datalist={datalist}")

        try:
            sw, result = super().readBinaryPlain(p1, p2, data)
            return 0x9000, result
        except Exception as e:
            logger.exception(f"{e}")


    def selectFile(self, p1, p2, data):
        """
        Function for instruction 0xa4. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string. Returns the status bytes as two
        byte long integer and the response data as binary string.
        """
        logger.debug(f"SelectFile: fid=0x{hexlify(data).decode('latin')} p2={p2}")

        P1_MF_DF_EF = p1 == 0
        P1_CHILD_DF = p1 & 0x01 == 0x01
        P1_EF_IN_DF = p1 & 0x02 == 0x02
        P1_PARENT_DF = p1 & 0x03 == 0x03
        P1_BY_DFNAME = p1 & 0x4 != 0 
        P1_DIRECT_BY_DFNAME = p1 & 0x7 == 0x04
        P1_BY_PATH = p1 & 0x8 != 0
        P1_FROM_MF = p1 & 0xf == 0x08
        P1_FROM_CURR_DF = p1  & 0xf == 0x09

        P2_FCI = p2 & 0x0C == 0
        P2_FCP = p2 & 0x04 != 0
        P2_FMD = p2 & 0x08 != 0
        P2_NONE = p2 & 0x0C != 0


        logger.debug(f"P1 - MF_DF_EF:{P1_MF_DF_EF} CHILD_DF:{P1_CHILD_DF} EF_IN_DF:{P1_EF_IN_DF} PARENT_DF:{P1_PARENT_DF} BY_DFNAME:{P1_BY_DFNAME} DIR_BY_DFNAME:{P1_DIRECT_BY_DFNAME} BY_PATH:{P1_BY_PATH} FROM_MF:{P1_FROM_MF} FROM_CUR_DF:{P1_FROM_CURR_DF}")
        logger.debug(f"P2 - FCI:{P2_FCI}, FCP:{P2_FCP} FMD:{P2_FMD} NONE:{P2_NONE}")
       
        # Patch instruction to Find MF to replicate PTEID behavior
        if data == b'\x4f\x00':
            p1 |= 8

        # Patch Applet AID
        if data == b'\x60\x46\x32\xFF\x00\x00\x02':
            return 0x9000, b''

        # Will fail with exception if File Not Found
        file = self._selectFile(p1, p2, data)
        self.current = file
        extra = b''
       
        if P2_NONE:
            pass
        elif P2_FCI:
            extra = self.get_fci(file)
            logger.debug(f"FCI: {extra}")
        else:
            extra = b''
    
        
        if isinstance(file, EF):
            logger.debug("IS EF")

        elif isinstance(file, DF):
            logger.debug("IS DF")
        
        return 0x9000, extra

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
                logger.debug(f"FCI Data: {file.extra_fci_data}")
                name = getattr(file, 'dfname')
                if len(name) > 0:
                    fcid = fcid + b'\x84' + len(name).to_bytes(1, byteorder='big') + name

            return fcid
        except Exception as e:
            logger.exception(f"get fci: {e}")
            return b''

# }}}
