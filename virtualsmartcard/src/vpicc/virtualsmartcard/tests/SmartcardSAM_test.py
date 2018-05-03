#
# Copyright (C) 2014 Dominik Oepen
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

import unittest
from virtualsmartcard.SmartcardSAM import *


class TestSmartcardSAM(unittest.TestCase):

    def setUp(self):
        self.password = "DUMMYKEYDUMMYKEY".encode('ascii')
        self.myCard = SAM("1234".encode('ascii'), "1234567890".encode('ascii'))
        self.secEnv = Security_Environment(None, self.myCard)  # TODO: Set CRTs
        self.secEnv.ht.algorithm = "SHA"
        self.secEnv.ct.algorithm = "AES-CBC"

    def test_incorrect_pin(self):
        with self.assertRaises(SwError):
            self.myCard.verify(0x00, 0x00, "5678".encode('ascii'))

    def test_counter_decrement(self):
        ctr1 = self.myCard.counter
        try:
            self.myCard.verify(0x00, 0x00, "3456".encode('ascii'))
        except SwError as e:
            pass
        self.assertEquals(self.myCard.counter, ctr1 - 1)

    def test_internal_authenticate(self):
        sw, challenge = self.myCard.get_challenge(0x00, 0x00, b"")
        blocklen = vsCrypto.get_cipher_blocklen("DES3-ECB")
        padded = vsCrypto.append_padding(blocklen, challenge)
        sw, result_data = self.myCard.internal_authenticate(0x00, 0x00, padded)
        self.assertEquals(sw, SW["NORMAL"])

    def test_external_authenticate(self):
        sw, challenge = self.myCard.get_challenge(0x00, 0x00, b"")
        blocklen = vsCrypto.get_cipher_blocklen("DES3-ECB")
        padded = vsCrypto.append_padding(blocklen, challenge)
        sw, result_data = self.myCard.internal_authenticate(0x00, 0x00, padded)
        sw, result_data = self.myCard.external_authenticate(0x00, 0x00,
                                                            result_data)
        self.assertEquals(sw, SW["NORMAL"])

    def test_security_environment(self):
        hash = self.secEnv.hash(0x90, 0x80, self.password)
        # The API should be changed so that the hash function returns SW_NORMAL
        self.secEnv.ct.key = hash[:16]
        crypted = self.secEnv.encipher(0x00, 0x00,
                                       self.password)
        # The API should be changed so that encipher() returns SW_NORMAL
        plain = self.secEnv.decipher(0x00, 0x00, crypted)
        # The API should be changed so that decipher() returns SW_NORMAL
        # self.assertEqual(plain, self.password)
        # secEnv.decipher doesn't strip padding. Should it?

        # should this really be secEnv.ct? probably rather secEnv.dst
        self.secEnv.ct.algorithm = "RSA"
        self.secEnv.dst.keylength = 1024
        sw, pk = self.secEnv.generate_public_key_pair(0x00, 0x00, b"")
        self.assertEquals(sw, SW["NORMAL"])


if __name__ == "__main__":
    unittest.main()

    # CF = CryptoflexSE(None)
    # print CF.generate_public_key_pair(0x00, 0x80, "\x01\x00\x01\x00")
    # print MyCard._get_referenced_key(0x01)
