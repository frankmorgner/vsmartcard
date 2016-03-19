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
import virtualsmartcard.CryptoUtils as vsCrypto
from virtualsmartcard.SWutils import SwError, SW
from virtualsmartcard.utils import stringtoint

import hashlib
import struct
from os import urandom


class ePass_SE(Security_Environment):
    """This class implements the Security Environment of the ICAO Passports. It
    is required in order to use the send sequence counter for secure messaging.
    """
    def __init__(self, MF, SAM, ssc=None):
        Security_Environment.__init__(self, MF, SAM)
        self.ssc = ssc
        self.cct.algorithm = "CC"
        self.cct.blocklength = 8
        self.ct.algorithm = "DES3-CBC"

    def compute_cryptographic_checksum(self, p1, p2, data):
        """
        Compute a cryptographic checksum (e.g. MAC) for the given data.
        The ePass uses a Send Sequence Counter for MAC calculation
        """
        if p1 != 0x8E or p2 != 0x80:
            raise SwError(SW["ERR_INCORRECTP1P2"])

        self.ssc += 1
        checksum = vsCrypto.crypto_checksum(self.cct.algorithm, self.cct.key,
                                            data, self.cct.iv, self.ssc)

        return checksum


class PassportSAM(SAM):
    """
    SAM for ICAO ePassport. Implements Basic access control and key derivation
    for Secure Messaging.
    """
    def __init__(self, mf):
        import virtualsmartcard.SmartcardFilesystem as vsFS

        SAM.__init__(self, None, None, mf, default_se=ePass_SE)

        ef_dg1 = vsFS.walk(mf, "\x00\x04\x01\x01")
        dg1 = ef_dg1.readbinary(5)
        self.mrz1 = dg1[:43]
        self.mrz2 = dg1[44:]
        self.KSeed = None
        self.KEnc = None
        self.KMac = None
        self.__computeKeys()

    def __computeKeys(self):
        """
        Computes the keys depending on the machine readable
        zone of the passport according to TR-PKI mrtds ICC read-only
        access v1.1 annex E.1.
        """

        MRZ_information = self.mrz2[0:10] + self.mrz2[13:20] + self.mrz2[21:28]
        H = hashlib.sha1(MRZ_information).digest()  # pylint: disable=E1101
        self.KSeed = H[:16]
        self.KEnc = self.derive_key(self.KSeed, 1)
        self.KMac = self.derive_key(self.KSeed, 2)

    @staticmethod
    def derive_key(seed, c):
        """
        Derive a key according to TR-PKI mrtds ICC read-only access v1.1
        annex E.1.
        c is either 1 for encryption or 2 for MAC computation.
        Returns: Ka + Kb
        Note: Does not adjust parity. Nobody uses that anyway ..."""
        D = seed + struct.pack(">i", c)
        H = hashlib.sha1(D).digest()  # pylint: disable=E1101
        Ka = H[0:8]
        Kb = H[8:16]
        return Ka + Kb

    def external_authenticate(self, p1, p2, resp_data):
        """Performs the basic access control protocol as defined in
        the ICAO MRTD standard"""
        rnd_icc = self.last_challenge

        # Receive Mutual Authenticate APDU from terminal
        # Decrypt data and check MAC
        Eifd = resp_data[:-8]
        padded_Eifd = vsCrypto.append_padding(self.current_SE.cct.blocklength,
                                              Eifd)
        Mifd = vsCrypto.crypto_checksum("CC", self.KMac, padded_Eifd)
        # Check the MAC
        if not Mifd == resp_data[-8:]:
            raise SwError(SW["ERR_SECMESSOBJECTSINCORRECT"])
        # Decrypt the data
        plain = vsCrypto.decrypt("DES3-CBC", self.KEnc, resp_data[:-8])
        if plain[8:16] != rnd_icc:
            raise SwError(SW["WARN_NOINFO63"])
        # Extract keying material from IFD, generate ICC keying material
        Kifd = plain[16:]
        rnd_ifd = plain[:8]
        Kicc = urandom(16)
        # Generate Answer
        data = plain[8:16] + plain[:8] + Kicc
        Eicc = vsCrypto.encrypt("DES3-CBC", self.KEnc, data)
        padded_Eicc = vsCrypto.append_padding(self.current_SE.cct.blocklength,
                                              Eicc)
        Micc = vsCrypto.crypto_checksum("CC", self.KMac, padded_Eicc)
        # Derive the final keys and set the current SE
        KSseed = vsCrypto.operation_on_string(Kicc, Kifd, lambda a, b: a ^ b)
        self.current_SE.ct.key = self.derive_key(KSseed, 1)
        self.current_SE.cct.key = self.derive_key(KSseed, 2)
        self.current_SE.ssc = stringtoint(rnd_icc[-4:] + rnd_ifd[-4:])
        return SW["NORMAL"], Eicc + Micc
