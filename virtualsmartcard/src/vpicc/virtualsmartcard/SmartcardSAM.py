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

import logging
from pickle import dumps, loads
from os import urandom

import virtualsmartcard.CryptoUtils as vsCrypto
from virtualsmartcard.SWutils import SwError, SW
from virtualsmartcard.utils import inttostring, stringtoint, hexdump
from virtualsmartcard.SEutils import Security_Environment


def get_referenced_cipher(p1):
    """
    P1 defines the algorithm and mode to use. We dispatch it and return a
    string that is understood by CryptoUtils.py functions
    """

    ciphertable = {
        0x00: "DES3-ECB",
        0x01: "DES3-ECB",
        0x02: "DES3-CBC",
        0x03: "DES-ECB",
        0x04: "DES-CBC",
        0x05: "AES-ECB",
        0x06: "AES-CBC",
        0x07: "RSA",
        0x08: "DSA"
    }

    if (p1 in ciphertable):
        return ciphertable[p1]
    else:
        raise SwError(SW["ERR_INCORRECTP1P2"])


class SAM(object):
    """
    This class is used to store the data needed by the SAM.
    It includes the PIN, the master key of the SAM and a hashmap containing all
    the keys used by the file encryption system. The keys in the hashmap are
    indexed via the path to the corresponding container.
    """

    def __init__(self, PIN, cardNumber, mf=None, cardSecret=None,
                 default_se=Security_Environment):

        self.PIN = PIN
        self.mf = mf
        self.cardNumber = cardNumber

        self.last_challenge = None  # Will contain non-readable binary string
        self.counter = 3  # Number of tries for PIN validation

        self.cipher = 0x01
        self.asym_key = None

        keylen = vsCrypto.get_cipher_keylen(get_referenced_cipher(self.cipher))
        if cardSecret is None:  # Generate a random card secret
            self.cardSecret = urandom(keylen)
        else:
            if len(cardSecret) != keylen:
                raise ValueError("cardSecret has the wrong key length for: " +
                                 get_referenced_cipher(self.cipher))
            else:
                self.cardSecret = cardSecret

        # Security Environments may be saved to/retrieved from this dictionary
        self.saved_SEs = {}
        self.default_se = default_se
        self.current_SE = default_se(self.mf, self)

    def set_MF(self, mf):
        """
        Setter function for the internal reference to the Filesystem. The SAM
        needs a reference to the filesystem in order to store/retrieve keys.
        """
        self.mf = mf

    def FSencrypt(self, data):
        """
        Encrypt the given data, using the parameters stored in the SAM.
        Right now we do not encrypt the data. In memory encryption might or
        might not be added in a future version.
        """
        return data

    def FSdecrypt(self, data):
        """
        Decrypt the given data, using the parameters stored in the SAM.
        Right now we do not encrypt the data. In memory encryption might or
        might not be added in a future version.
        """
        return data

    def store_SE(self, SEID):
        """
        Stores the current Security environment in the secure access module.
        The SEID is used as a reference to identify the SE.
        """
        SEstr = dumps(self.current_SE)
        self.saved_SEs[SEID] = SEstr
        return SW["NORMAL"], ""

    def restore_SE(self, SEID):
        """
        Restores a Security Environment from the SAM and replaces the current
        SE with it.
        """

        if (SEID not in self.saved_SEs):
            raise SwError(SW["ERR_REFNOTUSABLE"])
        else:
            SEstr = self.saved_SEs[SEID]
            SE = loads(SEstr)
            if isinstance(SE, self.default_se):
                self.current_SE = SE
            else:
                raise SwError(SW["ERR_REFNOTUSABLE"])

        return SW["NORMAL"], ""

    def erase_SE(self, SEID):
        """
        Erases a Security Environment stored under SEID from the SAM
        """
        if (SEID not in self.saved_SEs):
            raise SwError(SW["ERR_REFNOTUSABLE"])
        else:
            del self.saved_SEs[SEID]

        return SW["NORMAL"], ""

    def set_asym_algorithm(self, cipher, keytype):
        """
        :param cipher: Public/private key object from used for encryption
        :param keytype: Type of the public key (e.g. RSA, DSA)
        """
        if keytype not in (0x07, 0x08):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        else:
            self.cipher = keytype
            self.asym_key = cipher

    def verify(self, p1, p2, PIN):
        """
        Authenticate the card user. Check if he entered a valid PIN.
        If the PIN is invalid decrement retry counter. If retry counter
        equals zero, block the card until reset with correct PUK
        """

        logging.debug("Received PIN: %s", PIN.strip())
        PIN = PIN.replace(b"\0", b"")  # Strip NULL characters

        if p1 != 0x00:
            raise SwError(SW["ERR_INCORRECTP1P2"])

        if self.counter > 0:
            if self.PIN == PIN:
                self.counter = 3
                return SW["NORMAL"], ""
            else:
                self.counter -= 1
                raise SwError(SW["WARN_NOINFO63"])
        else:
            raise SwError(SW["ERR_AUTHBLOCKED"])

    def change_reference_data(self, p1, p2, data):
        """
        Change the specified referenced data (e.g. CHV) of the card
        """

        data = data.replace("\0", "")  # Strip NULL characters
        self.PIN = data
        return SW["NORMAL"], ""

    def internal_authenticate(self, p1, p2, data):
        """
        Authenticate card to terminal. Encrypt the challenge of the terminal
        to prove key posession
        """

        if p1 == 0x00:  # No information given
            cipher = get_referenced_cipher(self.cipher)
        else:
            cipher = get_referenced_cipher(p1)

        if cipher == "RSA" or cipher == "DSA":
            crypted_challenge = self.asym_key.sign(data, "")
            crypted_challenge = crypted_challenge[0]
            crypted_challenge = inttostring(crypted_challenge)
        else:
            key = self._get_referenced_key(p1, p2)
            crypted_challenge = vsCrypto.encrypt(cipher, key, data)

        return SW["NORMAL"], crypted_challenge

    def external_authenticate(self, p1, p2, data):
        """
        Authenticate the terminal to the card. Check whether Terminal correctly
        encrypted the given challenge or not
        """
        if self.last_challenge is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        key = self._get_referenced_key(p1, p2)
        if p1 == 0x00:  # No information given
            cipher = get_referenced_cipher(self.cipher)
        else:
            cipher = get_referenced_cipher(p1)

        blocklen = vsCrypto.get_cipher_blocklen(cipher)
        reference = vsCrypto.append_padding(blocklen, self.last_challenge)
        reference = vsCrypto.encrypt(cipher, key, reference)
        if(reference == data):
            # Invalidate last challenge
            self.last_challenge is None
            return SW["NORMAL"], ""
        else:
            raise SwError(SW["WARN_NOINFO63"])
            # TODO: Counter for external authenticate?

    def mutual_authenticate(self, p1, p2, mutual_challenge):
        """
        Takes an encrypted challenge in the form
        'Terminal Challenge | Card Challenge | Card number'
        and checks it for validity. If the challenge is successful
        the card encrypts 'Card Challenge | Terminal challenge' and
        returns this value
        """

        key = self._get_referenced_key(p1, p2)
        card_number = self.get_card_number()

        if (key is None):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        if p1 == 0x00:  # No information given
            cipher = get_referenced_cipher(self.cipher)
        else:
            cipher = get_referenced_cipher(p1)

        if (cipher is None):
            raise SwError(SW["ERR_INCORRECTP1P2"])

        plain = vsCrypto.decrypt(cipher, key, mutual_challenge)
        last_challenge_len = len(self.last_challenge)
        terminal_challenge = plain[:last_challenge_len-1]
        card_challenge = plain[last_challenge_len:-len(card_number)-1]
        serial_number = plain[-len(card_number):]

        if terminal_challenge != self.last_challenge:
            raise SwError(SW["WARN_NOINFO63"])
        elif serial_number != card_number:
            raise SwError(SW["WARN_NOINFO63"])

        result = card_challenge + terminal_challenge
        return SW["NORMAL"], vsCrypto.encrypt(cipher, key, result)

    def get_challenge(self, p1, p2, data):
        """
        Generate a random number of maximum 8 Byte and return it.
        """
        if (p1 != 0x00 or p2 != 0x00):  # RFU
            raise SwError(SW["ERR_INCORRECTP1P2"])

        length = 8  # Length of the challenge in Byte
        self.last_challenge = urandom(length)
        logging.debug("Generated challenge: %s", self.last_challenge)

        return SW["NORMAL"], self.last_challenge

    def get_card_number(self):
        return SW["NORMAL"], inttostring(self.cardNumber)

    def _get_referenced_key(self, p1, p2):
        """
        This method returns the key specified by the p2 parameter. The key may
        be stored on the cards filesystem.

        :param p1: Specifies the algorithm to use.
        :param p2: Specifies a reference to the key to be used for encryption.

            == == == == == == == == ===========================================
            b8 b7 b6 b5 b4 b3 b2 b1 Meaning
            == == == == == == == == ===========================================
            0  0  0  0  0  0  0  0  No information is given
            0  -  -  -  -  -  -  -  Global reference data(e.g. MF specific key)
            1  -  -  -  -  -  -  -  Specific reference data(e.g. DF specific
                                    key)
            -  -  -  x  x  x  x  x  Number of the secret
            == == == == == == == == ===========================================

            Any other value RFU
        """

        key = None
        qualifier = p2 & 0x1F
        algo = get_referenced_cipher(p1)
        keylength = vsCrypto.get_cipher_keylen(algo)

        if (p2 == 0x00):  # No information given, use the global card key
            key = self.cardSecret
        # We treat global and specific reference data alike
        else:
            # Interpret qualifier as an short fid (try to read the key from FS)
            if self.mf is None:
                raise SwError(SW["ERR_REFNOTUSABLE"])
            df = self.mf.currentDF()
            fid = df.select("fid", stringtoint(qualifier))
            key = fid.readbinary(keylength)

        if key is not None:
            return key
        else:
            raise SwError(SW["ERR_REFNOTUSABLE"])

    # The following commands define the Secure Messaging interface
    def generate_public_key_pair(self, p1, p2, data):
        return self.current_SE.generate_public_key_pair(p1, p2, data)

    def parse_SM_CAPDU(self, CAPDU, header_authentication):
        """
        Parse a command APDU protected by Secure Messaging and return the
        unprotected command APDU
        """
        try:
            return self.current_SE.parse_SM_CAPDU(CAPDU, header_authentication)
        except:
            raise SwError(SW["ERR_SECMESSOBJECTSINCORRECT"])

    def protect_result(self, sw, unprotected_result):
        """
        Protect a plain response APDU by Secure Messaging
        """
        logging.info("Unprotected Response Data:\n" +
                     hexdump(unprotected_result))
        return self.current_SE.protect_response(sw, unprotected_result)

    def perform_security_operation(self, p1, p2, data):
        return self.current_SE.perform_security_operation(p1, p2, data)

    def manage_security_environment(self, p1, p2, data):
        return self.current_SE.manage_security_environment(p1, p2, data)
