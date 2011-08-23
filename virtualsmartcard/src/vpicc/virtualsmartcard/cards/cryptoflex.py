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
from virtualsmartcard.utils import inttostring
from virtualsmartcard import TLVutils

import struct, logging

class CryptoflexSE(Security_Environment):
    def __init__(self, mf):
        Security_Environment.__init__(self, mf)

    def generate_public_key_pair(self, p1, p2, data):
        """
        In the Cryptoflex card this command only supports RSA keys.

        @param data: Contains the public exponent used for key generation
        @param p1: The keynumber. Can be used later to refer to the generated key
        @param p2: Used to specify the keylength.
                  The mapping is: 0x40 => 256 Bit, 0x60 => 512 Bit, 0x80 => 1024
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
        SAM.__init__(self, None, None, mf)
        self.current_SE = CryptoflexSE(mf)
        
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
        @param data: Contains the key to be verified
        @return: SW[NORMAL] in case of success otherwise SW[WARN_NOINFO63] 
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