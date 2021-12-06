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

from time import time
from random import seed, randint

from virtualsmartcard.TLVutils import pack, unpack, bertlv_pack
from virtualsmartcard.ConstantDefinitions import CRT_TEMPLATE, ALGO_MAPPING
from virtualsmartcard.ConstantDefinitions import SM_Class, MAX_EXTENDED_LE
from virtualsmartcard.utils import inttostring, stringtoint, C_APDU
from virtualsmartcard.SWutils import SwError, SW
import virtualsmartcard.CryptoUtils as vsCrypto
import logging

class ControlReferenceTemplate:
    """
    Control Reference Templates are used to configure the Security
    Environments. They specify which algorithms to use in which mode of
    operation and with which keys. There are six different types of Control
    Reference Template:
    HT, AT, KT, CCT, DST, CT-sym, CT-asym.
    """
    def __init__(self, tag, config=b""):
        """
        Generates a new CRT

        :param tag: Indicates the type of the CRT (HT, AT, KT, CCT, DST,
                    CT-sym, CT-asym)
        :param config: A string containing TLV encoded Security Environment
                       parameters
        """
        if tag not in (CRT_TEMPLATE["AT"], CRT_TEMPLATE["HT"],
                       CRT_TEMPLATE["KAT"], CRT_TEMPLATE["CCT"],
                       CRT_TEMPLATE["DST"], CRT_TEMPLATE["CT"]):
            raise ValueError("Unknown control reference tag.")
        else:
            self.type = tag

        self.iv = None
        self.keyref_secret_key = None
        self.keyref_public_key = None
        self.keyref_private_key = None
        self.key = None
        self.fileref = None
        self.DFref = None
        self.keylength = None
        self.algorithm = None
        self.blocklength = None
        self.usage_qualifier = None
        if config != b'':
            self.parse_SE_config(config)
        self.__config_string = config

    def parse_SE_config(self, config):
        """
        Parse a control reference template as given e.g. in an MSE APDU.

        :param config: a TLV string containing the configuration for the CRT.
        """
        error = False
        structure = unpack(config)
        for tlv in structure:
            tag, length, value = tlv
            if tag == 0x80:
                self.__set_algo(value)
            elif tag in (0x81, 0x82, 0x83, 0x84):
                self.__set_key(tag, value)
            elif tag in range(0x85, 0x93):
                self.__set_iv(tag, length, value)
            elif tag == 0x95:
                self.usage_qualifier = value
            else:
                error = True

        if error:
            raise SwError(SW["ERR_REFNOTUSABLE"])
        else:
            return SW["NORMAL"], ""

    def __set_algo(self, data):
        """
        Set the algorithm to be used by this CRT. The algorithms are specified
        in a global dictionary. New cards may add or modify this table in order
        to support new or different algorithms.

        :param data: reference to an algorithm
        """

        if data not in ALGO_MAPPING:
            raise SwError(SW["ERR_REFNOTUSABLE"])
        else:
            self.algorithm = ALGO_MAPPING[data]
            self.__replace_tag(0x80, data)

    def __set_key(self, tag, value):
        if tag == 0x81:
            self.fileref = value
        elif tag == 0x82:
            self.DFref = value
        elif tag == 0x83:
            self.keyref_secret_key = value
            self.keyref_public_key = value
        elif tag == 0x84:
            self.keyref_private_key = value

    def __set_iv(self, tag, length, value):
        iv = None
        if tag == 0x85:
            iv = 0x00
        elif tag == 0x86:
            pass  # What is initial chaining block?
        elif tag == 0x87 or tag == 0x93:
            if length != 0:
                iv = value
            else:
                iv = self.iv + 1
        elif tag == 0x91:
            if length != 0:
                iv = value
            else:
                seed(time())
                iv = hex(randint(0, 255))
        elif tag == 0x92:
            if length != 0:
                iv = value
            else:
                iv = int(time())
        self.iv = iv

    def __replace_tag(self, tag, data):
        """
        Adjust the config string using a given tag, value combination. If the
        config string already contains a tag, value pair for the given tag,
        replace it. Otherwise append tag, length and value to the config
        string.
        """

        position = 0
        while position < len(self.__config_string) and \
                self.__config_string[position] != tag:
            length = stringtoint(self.__config_string[position+1])
            position += length + 3

        if position < len(self.__config_string):  # Replace Tag
            length = stringtoint(chr(self.__config_string[position+1]))
            self.__config_string = self.__config_string[:position] +\
                inttostring(tag) + inttostring(len(data)) + data +\
                self.__config_string[position+2+length:]
        else:  # Add new tag
            self.__config_string += inttostring(tag) + inttostring(len(data)) + data

    def to_string(self):
        """
        Return the content of the CRT, encoded as TLV data in a string
        """
        return self.__config_string

    def __str__(self):
        return self.__config_string


class Security_Environment(object):

    def __init__(self, MF, SAM):
        self.sam = SAM
        self.mf = MF

        self.SEID = None

        # Control Reference Tables
        self.at = ControlReferenceTemplate(CRT_TEMPLATE["AT"])
        self.kat = ControlReferenceTemplate(CRT_TEMPLATE["KAT"])
        self.ht = ControlReferenceTemplate(CRT_TEMPLATE["HT"])
        self.cct = ControlReferenceTemplate(CRT_TEMPLATE["CCT"])
        self.dst = ControlReferenceTemplate(CRT_TEMPLATE["DST"])
        self.ct = ControlReferenceTemplate(CRT_TEMPLATE["CT"])

        self.capdu_sm = False
        self.rapdu_sm = False
        self.internal_auth = False
        self.external_auth = False

    def manage_security_environment(self, p1, p2, data):
        """
        This method is used to store, restore or erase Security Environments
        or to manipulate the various parameters of the current SE.
        P1 specifies the operation to perform, p2 is either the SEID for the
        referred SE or the tag of a control reference template

        :param p1:
            Bitmask according to this table

            == == == == == == == == =======================================
            b8 b7 b6 b5 b4 b3 b2 b1               Meaning
            == == == == == == == == =======================================
            -  -  -  1  -  -  -  -  Secure messaging in command data field
            -  -  1  -  -  -  -  -  Secure messaging in response data field
            -  1  -  -  -  -  -  -  Computation, decipherment, internal
                                    authentication and key agreement
            1  -  -  -  -  -  -  -  Verification, encipherment, external
                                    authentication and key agreement
            -  -  -  -  0  0  0  1  SET
            1  1  1  1  0  0  1  0  STORE
            1  1  1  1  0  0  1  1  RESTORE
            1  1  1  1  0  1  0  0  ERASE
            == == == == == == == == =======================================

        """

        cmd = p1 & 0x0F
        se = p1 >> 4
        if(cmd == 0x01):
            # Secure messaging in command data field
            if se & 0x01:
                self.capdu_sm = True
            # Secure messaging in response data field
            if se & 0x02:
                self.rapdu_sm = True
            # Computation, decryption, internal authentication and key
            # agreement
            if se & 0x04:
                self.internal_auth = True
            # Verification, encryption, external authentication and key
            # agreement
            if se & 0x08:
                self.external_auth = True
            return self._set_SE(p2, data)
        elif cmd == 0x02:
            return self.sam.store_SE(p2)
        elif cmd == 0x03:
            return self.sam.restore_SE(p2)
        elif cmd == 0x04:
            return self.sam.erase_SE(p2)
        else:
            raise SwError(SW["ERR_INCORRECTP1P2"])

    def _set_SE(self, p2, data):
        """
        Manipulate the current Security Environment. P2 is the tag of a
        control reference template, data contains control reference objects
        """

        if p2 == 0xA4:
            return self.at.parse_SE_config(data)
        elif p2 == 0xA6:
            return self.kat.parse_SE_config(data)
        elif p2 == 0xAA:
            return self.ht.parse_SE_config(data)
        elif p2 == 0xB4:
            return self.cct.parse_SE_config(data)
        elif p2 == 0xB6:
            return self.dst.parse_SE_config(data)
        elif p2 == 0xB8:
            return self.ct.parse_SE_config(data)

        raise SwError(SW["ERR_INCORRECTP1P2"])

    def parse_SM_CAPDU(self, CAPDU, authenticate_header):
        """
        This methods parses a data field including Secure Messaging objects.
        SM_header indicates whether or not the header of the message shall be
        authenticated. It returns an unprotected command APDU

        :param CAPDU: The protected CAPDU to be parsed
        :param authenticate_header: Whether or not the header should be
            included in authentication mechanisms
        :returns: Unprotected command APDU
        """

        structure = unpack(CAPDU.data)
        return_data = ["", ]

        cla = None
        ins = None
        p1 = None
        p2 = None
        le = None

        if authenticate_header:
            to_authenticate = inttostring(CAPDU.cla) +\
                inttostring(CAPDU.ins) + inttostring(CAPDU.p1) +\
                inttostring(CAPDU.p2)
            to_authenticate = vsCrypto.append_padding(self.cct.blocklength,
                                                      to_authenticate)
        else:
            to_authenticate = ""

        for tlv in structure:
            tag, length, value = tlv

            if tag % 2 == 1:  # Include object in checksum calculation
                to_authenticate += bertlv_pack([[tag, length, value]])

            # SM data objects for encapsulating plain values
            if tag in (SM_Class["PLAIN_VALUE_NO_TLV"],
                       SM_Class["PLAIN_VALUE_NO_TLV_ODD"]):
                return_data.append(value)  # FIXME: Need TLV coding?
            # Encapsulated SM objects. Parse them
            # FIXME: Need to pack value into a dummy CAPDU
            elif tag in (SM_Class["PLAIN_VALUE_TLV_INCULDING_SM"],
                         SM_Class["PLAIN_VALUE_TLV_INCULDING_SM_ODD"]):
                return_data.append(self.parse_SM_CAPDU(value,
                                                       authenticate_header))
            # Encapsulated plaintext BER-TLV objects
            elif tag in (SM_Class["PLAIN_VALUE_TLV_NO_SM"],
                         SM_Class["PLAIN_VALUE_TLV_NO_SM_ODD"]):
                return_data.append(value)
            elif tag in (SM_Class["Ne"], SM_Class["Ne_ODD"]):
                le = value
            elif tag == SM_Class["PLAIN_COMMAND_HEADER"]:
                if len(value) != 8:
                    raise SwError(SW["ERR_SECMESSOBJECTSINCORRECT"])
                else:
                    cla = value[:2]
                    ins = value[2:4]
                    p1 = value[4:6]
                    p2 = value[6:8]

            # SM data objects for confidentiality
            if tag in (SM_Class["CRYPTOGRAM_PLAIN_TLV_INCLUDING_SM"],
                       SM_Class["CRYPTOGRAM_PLAIN_TLV_INCLUDING_SM_ODD"]):
                # The cryptogram includes SM objects.
                # We decrypt them and parse the objects.
                plain = self.decipher(tag, 0x80, value)
                # TODO: Need Le = length
                return_data.append(self.parse_SM_CAPDU(plain,
                                                       authenticate_header))
            elif tag in (SM_Class["CRYPTOGRAM_PLAIN_TLV_NO_SM"],
                         SM_Class["CRYPTOGRAM_PLAIN_TLV_NO_SM_ODD"]):
                # The cryptogram includes BER-TLV encoded plaintext.
                # We decrypt them and return the objects.
                plain = self.decipher(tag, 0x80, value)
                return_data.append(plain)
            elif tag in (SM_Class["CRYPTOGRAM_PADDING_INDICATOR"],
                         SM_Class["CRYPTOGRAM_PADDING_INDICATOR_ODD"]):
                # The first byte of the data field indicates the padding to use
                """
                Value   Meaning
                '00'    No further indication
                '01'    Padding as specified in 6.2.3.1
                '02'    No padding
                '1X'    One to four secret keys for enciphering information,
                        not keys ('X' is a bitmap with any value from '0' to
                        'F')
                '11'    indicates the first key (e.g., an "even" control word
                        in a pay TV system)
                '12'    indicates the second key (e.g., an "odd" control word
                        in a pay TV system)
                '13'    indicates the first key followed by the second key
                        (e.g., a pair of control words in a pay TV system)
                '2X'    Secret key for enciphering keys, not information
                        ('X' is a reference with any value from '0' to 'F')
                        (e.g., in a pay TV system, either an operational key
                        for enciphering control words, or a management key for
                        enciphering operational keys)
                '3X'    Private key of an asymmetric key pair ('X' is a
                        reference with any value from '0' to 'F')
                '4X'    Password ('X' is a reference with any value from '0' to
                        'F')
            '80' to '8E' Proprietary
                """
                padding_indicator = stringtoint(value[0])
                plain = self.decipher(tag, 0x80, value[1:])
                plain = vsCrypto.strip_padding(self.ct.blocklength, plain,
                                               padding_indicator)
                return_data.append(plain)

            # SM data objects for authentication
            if tag == SM_Class["CHECKSUM"]:
                auth = vsCrypto.append_padding(self.cct.blocklength,
                                               to_authenticate)
                checksum = self.compute_cryptographic_checksum(0x8E, 0x80,
                                                               auth)
                if checksum != value:
                    raise SwError(SW["ERR_SECMESSOBJECTSINCORRECT"])
            elif tag == SM_Class["DIGITAL_SIGNATURE"]:
                auth = to_authenticate  # FIXME: Need padding?
                signature = self.compute_digital_signature(0x9E, 0x9A, auth)
                if signature != value:
                    raise SwError(SW["ERR_SECMESSOBJECTSINCORRECT"])
            elif tag in (SM_Class["HASH_CODE"], SM_Class["HASH_CODE_ODD"]):
                hash = self.hash(p1, p2, to_authenticate)
                if hash != value:
                    raise SwError(SW["ERR_SECMESSOBJECTSINCORRECT"])

        # Form unprotected CAPDU
        if cla is None:
            cla = CAPDU.cla
        if ins is None:
            ins = CAPDU.ins
        if p1 is None:
            p1 = CAPDU.p1
        if p2 is None:
            p2 = CAPDU.p2
        # FIXME:
        # if expected != "":
        #    raise SwError(SW["ERR_SECMESSOBJECTSMISSING"])

        if isinstance(le, bytes):
            # FIXME: C_APDU only handles le with strings of length 1.
            # Better patch utils.py to support extended length apdus
            le_int = stringtoint(le)
            if le_int == 0 and len(le) > 1:
                le_int = MAX_EXTENDED_LE
            le = le_int

        c = C_APDU(cla=cla, ins=ins, p1=p1, p2=p2, le=le,
                   data="".join(return_data))
        return c

    def protect_response(self, sw, result):
        """
        This method protects a response APDU using secure messaging mechanisms

        :returns: the protected data and the SW bytes
        """

        return_data = ""
        # if sw == SW["NORMAL"]:
        #    sw = inttostring(sw)
        #    length = len(sw)
        #    tag = SM_Class["PLAIN_PROCESSING_STATUS"]
        #    tlv_sw = pack([(tag,length,sw)])
        #    return_data += tlv_sw

        if result != "":  # Encrypt the data included in the RAPDU
            encrypted = self.encipher(0x82, 0x80, result)
            encrypted = "\x01" + encrypted
            encrypted_tlv = pack([(
                                SM_Class["CRYPTOGRAM_PADDING_INDICATOR_ODD"],
                                len(encrypted),
                                encrypted)])
            return_data += encrypted_tlv

        if sw == SW["NORMAL"]:
            if self.cct.algorithm is None:
                raise SwError(SW["CONDITIONSNOTSATISFIED"])
            elif self.cct.algorithm == "CC":
                tag = SM_Class["CHECKSUM"]
                padded = vsCrypto.append_padding(self.cct.blocklength,
                                                 return_data)
                auth = self.compute_cryptographic_checksum(0x8E, 0x80, padded)
                length = len(auth)
                return_data += pack([(tag, length, auth)])
            elif self.cct.algorithm == "SIGNATURE":
                tag = SM_Class["DIGITAL_SIGNATURE"]
                hash = self.hash(0x90, 0x80, return_data)
                auth = self.compute_digital_signature(0x9E, 0x9A, hash)
                length = len(auth)
                return_data += pack([(tag, length, auth)])

        return sw, return_data

    # The following commands implement ISO 7816-8 {{{
    def perform_security_operation(self, p1, p2, data):
        """
        In the end this command is nothing but a big switch for all the other
        commands in ISO 7816-8. It will invoke the appropriate command and
        return its result
        """

        allowed_P1P2 = ((0x90, 0x80), (0x90, 0xA0), (0x9E, 0x9A), (0x9E, 0xAC),
                        (0x9E, 0xBC), (0x00, 0xA2), (0x00, 0xA8), (0x00, 0x92),
                        (0x00, 0xAE), (0x00, 0xBE), (0x82, 0x80), (0x84, 0x80),
                        (0x86, 0x80), (0x80, 0x82), (0x80, 0x84), (0x80, 0x86))

        if (p1, p2) not in allowed_P1P2:
            raise SwError(SW["INCORRECTP1P2"])

        if((p2 in (0x80, 0xA0)) and (p1 == 0x90)):
            response_data = self.hash(p1, p2, data)
        elif(p2 in (0x9A, 0xAC, 0xBC) and p1 == 0x9E):
            response_data = self.compute_digital_signature(p1, p2, data)
        elif(p2 == 0xA2 and p1 == 0x00):
            response_data = self.verify_cryptographic_checksum(p1, p2, data)
        elif(p2 == 0xA8 and p1 == 0x00):
            response_data = self.verify_digital_signature(p1, p2, data)
        elif(p2 in (0x92, 0xAE, 0xBE) and p1 == 0x00):
            response_data = self.verify_certificate(p1, p2, data)
        elif (p2 == 0x80 and p1 in (0x82, 0x84, 0x86)):
            response_data = self.encipher(p1, p2, data)
        elif (p2 in (0x82, 0x84, 0x86) and p1 == 0x80):
            response_data = self.decipher(p1, p2, data)

        if p1 == 0x00:
            assert response_data == ""

        return SW["NORMAL"], response_data

    def compute_cryptographic_checksum(self, p1, p2, data):
        """
        Compute a cryptographic checksum (e.g. MAC) for the given data.
        Algorithm and key are specified in the current SE
        """
        if p1 != 0x8E or p2 != 0x80:
            raise SwError(SW["ERR_INCORRECTP1P2"])
        if self.cct.key is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        checksum = vsCrypto.crypto_checksum(self.cct.algorithm, self.cct.key,
                                            data, self.cct.iv)
        return checksum

    def compute_digital_signature(self, p1, p2, data):
        """
        Compute a digital signature for the given data.
        Algorithm and key are specified in the current SE

        :param p1: Must be 0x9E = Secure Messaging class for digital signatures
        :param p2: Must be one of 0x9A, 0xAC, 0xBC. Indicates what kind of data
            is included in the data field.
        """

        if p1 != 0x9E or p2 not in (0x9A, 0xAC, 0xBC):
            raise SwError(SW["ERR_INCORRECTP1P2"])

        if self.dst.key is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        to_sign = ""
        if p2 == 0x9A:  # Data to be signed
            to_sign = data
        elif p2 == 0xAC:  # Data objects, sign values
            to_sign = ""
            structure = unpack(data)
            for tag, length, value in structure:
                to_sign += value
        elif p2 == 0xBC:  # Data objects to be signed
            pass

        signature = self.dst.key.sign(to_sign, "")
        return signature

    def hash(self, p1, p2, data):
        """
        Hash the given data using the algorithm specified by the current
        Security environment.

        :return: raw data (no TLV coding).
        """
        if p1 != 0x90 or p2 not in (0x80, 0xA0):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        algo = self.ht.algorithm
        if algo is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])
        try:
            hash = vsCrypto.hash(algo, data)
        except ValueError:
            raise SwError(SW["ERR_EXECUTION"])

        return hash

    def verify_cryptographic_checksum(self, p1, p2, data):
        """
        Verify the cryptographic checksum contained in the data field.
        Data field must contain a cryptographic checksum (tag 0x8E) and a plain
        value (tag 0x80)
        """
        plain = ""
        cct = ""

        algo = self.cct.algorithm
        key = self.cct.key
        iv = self.cct.iv
        if algo is None or key is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        structure = unpack(data)
        for tag, length, value in structure:
            if tag == 0x80:
                plain = value
            elif tag == 0x8E:
                cct = value
        if plain == "" or cct == "":
            raise SwError(SW["ERR_SECMESSOBJECTSMISSING"])
        else:
            my_cct = vsCrypto.crypto_checksum(algo, key, plain, iv)
            if my_cct == cct:
                return ""
            else:
                raise SwError["ERR_SECMESSOBJECTSINCORRECT"]

    def verify_digital_signature(self, p1, p2, data):
        """
        Verify the digital signature contained in the data field. Data must
        contain a data to sign (tag 0x9A, 0xAC or 0xBC) and a digital signature
        (0x9E)
        """
        key = self.dst.key
        to_sign = ""
        signature = ""

        if key is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        structure = unpack(data)
        for tag, length, value in structure:
            if tag == 0x9E:
                signature = value
            elif tag == 0x9A:  # FIXME: Correct treatment of all possible tags
                to_sign = value
            elif tag == 0xAC:
                pass
            elif tag == 0xBC:
                pass

        if to_sign == "" or signature == "":
            raise SwError(SW["ERR_SECMESSOBJECTSMISSING"])

        my_signature = key.sign(value)
        if my_signature == signature:
            return ""
        else:
            raise SwError(["ERR_SECMESSOBJECTSINCORRECT"])

    def verify_certificate(self, p1, p2, data):
        """
        Verify a certificate send by the terminal using the internal trust
        anchors.
        This method is currently not implemented.
        """
        if p1 != 0x00 or p2 not in (0x92, 0xAE, 0xBE):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        else:
            raise SwError(SW["ERR_NOTSUPPORTED"])

    def encipher(self, p1, p2, data):
        """
        Encipher data using key, algorithm, IV and Padding specified
        by the current Security environment.

        :returns: raw data (no TLV coding).
        """
        algo = self.ct.algorithm
        key = self.ct.key
        if key is None or algo is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])
        else:
            blocklen = vsCrypto.get_cipher_blocklen(algo)
            padded = vsCrypto.append_padding(blocklen, data)
            crypted = vsCrypto.encrypt(algo, key, padded, self.ct.iv)
            return crypted

    def decipher(self, p1, p2, data):
        """
        Decipher data using key, algorithm, IV and Padding specified
        by the current Security environment.

        :returns: raw data (no TLV coding). Padding is not removed!!!
        """
        algo = self.ct.algorithm
        key = self.ct.key
        if key is None or algo is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])
        else:
            plain = vsCrypto.decrypt(algo, key, data, self.ct.iv)
            return plain

    def generate_public_key_pair(self, p1, p2, data):
        """
        The GENERATE PUBLIC-KEY PAIR command either initiates the generation
        and storing of a key pair, i.e., a public key and a private key, in the
        card, or accesses a key pair previously generated in the card.

        :param p1: should be 0x00 (generate new key)
        :param p2: '00' (no information provided) or reference of the key to
                   be generated
        :param data: One or more CRTs associated to the key generation if
                     P1-P2 different from '0000'
        """

        from Crypto.PublicKey import RSA, DSA

        cipher = self.ct.algorithm

        c_class = locals().get(cipher, None)
        if c_class is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        if p1 & 0x01 == 0x00:  # Generate key
            PublicKey = c_class.generate(self.dst.keylength)
            self.dst.key = PublicKey
        else:
            pass  # Read key

        # Encode keys
        if cipher == "RSA":
            # Public key
            n = inttostring(PublicKey.n)
            e = inttostring(PublicKey.e)
            pk = ((0x81, len(n), n), (0x82, len(e), e))
            result = bertlv_pack(pk)
            # Private key
            d = PublicKey.d
        elif cipher == "DSA":
            # DSAParams
            p = inttostring(PublicKey.p)
            q = inttostring(PublicKey.q)
            g = inttostring(PublicKey.g)
            # Public key
            y = inttostring(PublicKey.y)
            pk = ((0x81, len(p), p), (0x82, len(q), q), (0x83, len(g), g),
                  (0x84, len(y), y))
            # Private key
            x = inttostring(PublicKey.x)
        # Add more algorithms here
        # elif cipher = "ECDSA":
        else:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        result = bertlv_pack([[0x7F49, len(pk), pk]])
        # TODO: Internally store key pair

        if p1 & 0x02 == 0x02:
            # We do not support extended header lists yet
            raise SwError["ERR_NOTSUPPORTED"]
        else:
            return SW["NORMAL"], result
