#
# Copyright (C) 2011 Dominik Oepen
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
from virtualsmartcard.SEutils import Security_Environment
from virtualsmartcard.SWutils import SwError, SW
from virtualsmartcard.utils import inttostring, stringtoint
from virtualsmartcard import TLVutils

from virtualsmartcard.SmartcardFilesystem import MF, TransparentStructureEF, \
    RecordStructureEF, DF, EF
from virtualsmartcard.ConstantDefinitions import FDB

import struct, logging

class CryptoflexSE(Security_Environment):
    def __init__(self, MF, SAM):
        Security_Environment.__init__(self, MF, SAM)

    def generate_public_key_pair(self, p1, p2, data):
        """
        In the Cryptoflex card this command only supports RSA keys.

        :param data:
            Contains the public exponent used for key generation
        :param p1:
            The key number. Can be used later to refer to the generated key
        :param p2:
            Used to specify the key length. The mapping is: 0x40 => 256 Bit,
            0x60 => 512 Bit, 0x80 => 1024
        """
        from Crypto.PublicKey import RSA
        from Crypto.Util.randpool import RandomPool

        keynumber = p1 #TODO: Check if key exists

        keylength_dict = {0x40: 256, 0x60: 512, 0x80: 1024}

        if not keylength_dict.has_key(p2):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        else:
            keylength = keylength_dict[p2]

        rnd = RandomPool()
        PublicKey = RSA.generate(keylength, rnd.get_bytes)
        self.dst.key = PublicKey

        e_in = struct.unpack("<i", data)
        if e_in[0] != 65537:
            logging.warning("Warning: Exponents different from 65537 are ignored!" +\
                            "The Exponent given is %i" % e_in[0])

        #Encode Public key
        n = PublicKey.__getstate__()['n']
        n_str = inttostring(n)
        n_str = n_str[::-1]
        e = PublicKey.__getstate__()['e']
        e_str = inttostring(e, 4)
        e_str = e_str[::-1]
        pad = 187 * '\x30' #We don't have CRT components, so we need to pad
        pk_n = TLVutils.bertlv_pack(((0x81, len(n_str), n_str), 
                                     (0x01, len(pad), pad),
                                     (0x82, len(e_str), e_str)))
        #Private key
        d = PublicKey.__getstate__()['d']

        #Write result to FID 10 12 EF-PUB-KEY
        df = self.mf.currentDF()
        ef_pub_key = df.select("fid", 0x1012)
        ef_pub_key.writebinary([0], [pk_n])
        data = ef_pub_key.getenc('data')
        
        #Write private key to FID 00 12 EF-PRI-KEY (not necessary?)
        #How to encode the private key?
        ef_priv_key = df.select("fid", 0x0012)
        ef_priv_key.writebinary([0], [inttostring(d)]) 
        data = ef_priv_key.getenc('data')     
        return PublicKey


class CryptoflexSAM(SAM):
    def __init__(self, mf=None):
        SAM.__init__(self, None, None, mf, default_se=CryptoflexSE)
        self.current_SE = self.default_se(self.mf, self)
        
    def generate_public_key_pair(self, p1, p2, data):
        asym_key = self.current_SE.generate_public_key_pair(p1, p2, data)
        #TODO: Use SE instead (and remove SAM.set_asym_algorithm)
        self.set_asym_algorithm(asym_key, 0x07)
        return SW["NORMAL"], ""
    
    def perform_security_operation(self, p1, p2, data):
        """
        In the cryptoflex card, this is the verify key command. A key is send
        to the card in plain text and compared to a key stored in the card.
        This is used for authentication
        
        :param data: Contains the key to be verified
        :returns: SW[NORMAL] in case of success otherwise SW[WARN_NOINFO63] 
        """
        
        return SW["NORMAL"], ""
        #FIXME
        #key = self._get_referenced_key(p1,p2)
        #if key == data:
        #    return SW["NORMAL"], ""
        #else:
        #    return SW["WARN_NOINFO63"], ""
        
    def internal_authenticate(self, p1, p2, data):
        data = data[::-1] #Reverse Byte order
        sw, data = SAM.internal_authenticate(self, p1, p2, data)
        if data != "":
            data = data[::-1]
        return sw, data
    
class CryptoflexMF(MF): # {{{

    @staticmethod
    def create(p1, p2, data):

        if data[0:2] != "\xff\xff":
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        args = {
                "parent": None,
                "filedescriptor": 0,
                "fid": stringtoint(data[4:6]),
                }
        if data[6] == "\x01":
            args["data"] = chr(0)*stringtoint(data[2:4])
            args["filedescriptor"] = FDB["EFSTRUCTURE_TRANSPARENT"]
            new_file = TransparentStructureEF(**args)
        elif data[6] == "\x02":
            if len(data)>16:
                args["maxrecordsize"] = stringtoint(data[16])
            elif p2:
                # if given a number of records
                args["maxrecordsize"] = (stringtoint(data[2:4])/p2)
            args["filedescriptor"] = FDB["EFSTRUCTURE_LINEAR_FIXED_NOFURTHERINFO"]
            new_file = RecordStructureEF(**args)
        elif data[6] == "\x03":
            args["filedescriptor"] = FDB["EFSTRUCTURE_LINEAR_VARIABLE_NOFURTHERINFO"]
            new_file = RecordStructureEF(**args)
        elif data[6] == "\x04":
            args["filedescriptor"] = FDB["EFSTRUCTURE_CYCLIC_NOFURTHERINFO"]
            new_file = RecordStructureEF(**args)
        elif data[6] == "\x38":
            if data[12] != "\x03":
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])
            new_file = DF(**args)
        else:
            logging.error("unknown type: 0x%x" % ord(data[6]))
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])


        return [new_file]

    def recordHandlingDecode(self, p1, p2):
        if p2 < 4:
            p1 = 0

        return MF.recordHandlingDecode(self, p1, p2)


    def dataUnitsDecodePlain(self, p1, p2, data):
        ef = self.currentEF()
        # FIXME: Cyberflex Access TM Software Development Kit Release 3C
        # enforces the following:
        # offsets = [(p2 << 8) + p1]
        # but smartcard_login-0.1.1 requires:
        offsets = [(p1 << 8) + p2]

        datalist = [data]
        return ef, offsets, datalist

    def selectFile(self, p1, p2, data):
        """
        Function for instruction 0xa4. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string. Returns the status bytes as two
        byte long integer and the response data as binary string.
        """
        P2_FCI  = 0
        P2_FCP  = 1 << 2
        P2_FMD  = 2 << 2
        P2_NONE = 3 << 2
        file = self._selectFile(p1, p2, data)

        if isinstance(file, EF):
            size = inttostring(min(0xffff, len(file.getenc('data'))), 2) # File size (body only)
            extra = chr(0) # RFU
            if isinstance(file, RecordStructureEF) and file.hasFixedRecordSize() and not file.isCyclic():
                extra += chr(0) + chr(min(file.records, 0xff)) # Length of records
        elif isinstance(file, DF):
            size = inttostring(0xffff, 2) # Number of unused EEPROM bytes available in the DF
            efcount = 0
            dfcount = 0
            chv1 = None
            chv2 = None
            chv1lifecycle = 0
            chv2lifecycle = 0
            for f in self.content:
                if f.fid == 0x0000:
                    chv1 = f
                    chv1lifecycle = f.lifecycle & 1
                if f.fid == 0x0100:
                    chv2 = f
                    chv2lifecycle = f.lifecycle & 1
                if isinstance(f, EF):
                    efcount += 1
                if isinstance(f, DF):
                    dfcount += 1
            if chv1:
                extra = chr(1) # TODO LSB correct?
            else:
                extra = chr(0) # TODO LSB correct?
            extra += chr(efcount)
            extra += chr(dfcount)
            extra += chr(0) # TODO Number of PINs and unblock CHV PINs
            extra += chr(0) # RFU
            if chv1:
                extra += chr(0) # TODO Number of remaining CHV1 attempts
                extra += chr(0) # TODO Number of remaining unblock CHV1 attempts
                if chv2:
                    extra += chr(0) # TODO CHV2 key status
                    extra += chr(0) # TODO CHV2 unblocking key status

        data = inttostring(0, 2) # RFU
        data += size
        data += inttostring(file.fid, 2)
        data += inttostring(file.filedescriptor) # File type
        data += inttostring(0, 4) # ACs TODO
        data += chr(file.lifecycle & 1) # File status
        data += chr(len(extra))
        data += extra

        self.current = file

        return SW["NORMAL"], data
# }}}
