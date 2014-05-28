#
# Copyright (C) 2011 Dominik Oepen, Frank Morgner
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
from virtualsmartcard.SEutils import ControlReferenceTemplate, Security_Environment
from virtualsmartcard.SWutils import SwError, SW
from virtualsmartcard.ConstantDefinitions import CRT_TEMPLATE, SM_Class, ALGO_MAPPING
from virtualsmartcard.TLVutils import unpack, bertlv_pack
from virtualsmartcard.SmartcardFilesystem import make_property
from virtualsmartcard.utils import inttostring
import virtualsmartcard.CryptoUtils as vsCrypto
from chat import CHAT, CVC, PACE_SEC, EAC_CTX
import eac

class nPA_AT_CRT(ControlReferenceTemplate):

    PACE_MRZ = 0x01
    PACE_CAN = 0x02
    PACE_PIN = 0x03
    PACE_PUK = 0x04

    def __init__(self):
        ControlReferenceTemplate.__init__(self, CRT_TEMPLATE["AT"])
        self.chat = None

    def keyref_is_mrz(self):
        if self.keyref_secret_key == '%c'% self.PACE_MRZ:
            return True
        return False

    def keyref_is_can(self):
        if self.keyref_secret_key == '%c'% self.PACE_CAN:
            return True
        return False

    def keyref_is_pin(self):
        if self.keyref_secret_key == '%c'% self.PACE_PIN:
            return True
        return False

    def keyref_is_puk(self):
        if self.keyref_secret_key == '%c'% self.PACE_PUK:
            return True
        return False

    def parse_SE_config(self, config):
        r = 0x9000
        try:
            ControlReferenceTemplate.parse_SE_config(self, config)
        except SwError as e:
            structure = unpack(config)
            for tlv in structure:
                tag, length, value = tlv
                if tag == 0x7f4c:
                    self.chat = CHAT(bertlv_pack([[tag, length, value]]))
                    print(self.chat)
                elif tag == 0x67:
                    self.auxiliary_data = bertlv_pack([[tag, length, value]])
                elif tag == 0x80 or tag == 0x84 or tag == 0x83 or tag == 0x91:
                    # handled by ControlReferenceTemplate.parse_SE_config
                    pass
                else:
                    raise SwError(SW["ERR_REFNOTUSABLE"])

        return r, ""

class nPA_SE(Security_Environment):

    eac_step = make_property("eac_step", "next step to performed for EAC")

    def __init__(self, MF, SAM):
        Security_Environment.__init__(self, MF, SAM)
        self.at = nPA_AT_CRT() 
        #This breaks support for 3DES
        self.cct.blocklength = 16
        self.cct.algorithm = "CC"
        self.eac_step = 0
        self.sec = None
        self.eac_ctx = None
        self.cvca = None
        self.car = None
        self.ca_key = None
        self.disable_checks = False

    def _set_SE(self, p2, data):
        sw, resp = Security_Environment._set_SE(self, p2, data)
        
        if self.at.algorithm == "PACE":
            if self.at.keyref_is_pin():
                if self.sam.counter <= 0:
                    print "Must use PUK to unblock"
                    return 0x63c0, ""
                if self.sam.counter == 1 and not self.sam.active:
                    print "Must use CAN to activate"
                    return 0x63c1, ""
            self.eac_step = 0
        elif self.at.algorithm == "TA":
            if self.eac_step != 4:
                SwError(SW["ERR_AUTHBLOCKED"])
        elif self.at.algorithm == "CA":
            if self.eac_step != 5:
                SwError(SW["ERR_AUTHBLOCKED"])

        return sw, resp

    def general_authenticate(self, p1, p2, data):
        if (p1, p2) != (0x00, 0x00):
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        if   self.eac_step == 0 and self.at.algorithm == "PACE":
            return self.__eac_pace_step1(data)
        elif self.eac_step == 1 and self.at.algorithm == "PACE":
            return self.__eac_pace_step2(data)
        elif self.eac_step == 2 and self.at.algorithm == "PACE":
            return self.__eac_pace_step3(data)
        elif self.eac_step == 3 and self.at.algorithm == "PACE":
            return self.__eac_pace_step4(data)
        elif self.eac_step == 5 and self.at.algorithm == "CA":
            return self.__eac_ca(data)
        elif self.eac_step == 6:
            # TODO implement RI
            return SW["NORMAL"], "\x7c\x22\x81\x20\x48\x1e\x58\xd1\x7c\x12\x9a\x0a\xb4\x63\x7d\x43\xc7\xf7\xeb\x2b\x06\x10\x6f\x26\x90\xe3\x00\xc4\xe7\x03\x54\xa0\x41\xf0\xd3\x90"

        raise SwError(SW["ERR_INCORRECTPARAMETERS"])

    @staticmethod
    def __unpack_general_authenticate(data):
        data_structure = []
        structure = unpack(data)
        for tlv in structure:
            tag, length, value = tlv
            if tag == 0x7c:
                data_structure = value
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        return data_structure

    @staticmethod
    def __pack_general_authenticate(data):
        tlv_data = bertlv_pack(data)
        return bertlv_pack([[0x7c, len(tlv_data), tlv_data]])

    def __eac_pace_step1(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)
        if  tlv_data != []:
            raise SwError(SW["WARN_NOINFO63"])

        if self.at.keyref_is_mrz():
            self.PACE_SEC = PACE_SEC(self.sam.mrz, eac.PACE_MRZ)
        elif self.at.keyref_is_can():
            self.PACE_SEC = PACE_SEC(self.sam.can, eac.PACE_CAN)
        elif self.at.keyref_is_pin():
            if self.sam.counter <= 0:
                print "Must use PUK to unblock"
                return 0x63c0, ""
            if self.sam.counter == 1 and not self.sam.active:
                print "Must use CAN to activate"
                return 0x63c1, ""
            self.PACE_SEC = PACE_SEC(self.sam.PIN, eac.PACE_PIN)
            self.sam.counter -= 1
            if self.sam.counter <= 1:
                self.sam.active = False
        elif self.at.keyref_is_puk():
            if self.sam.counter_puk <= 0:
                raise SwError(SW["WARN_NOINFO63"])
            self.PACE_SEC = PACE_SEC(self.sam.puk, eac.PACE_PUK)
            self.sam.counter_puk -= 1
        else:
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        self.sec = self.PACE_SEC.sec

        if not self.eac_ctx:
            eac.EAC_init()

            self.EAC_CTX = EAC_CTX()
            self.eac_ctx = self.EAC_CTX.ctx
            eac.CA_disable_passive_authentication(self.eac_ctx)

            ef_card_security = self.mf.select('fid', 0x011d)
            ef_card_security_data = ef_card_security.data
            eac.EAC_CTX_init_ef_cardsecurity(ef_card_security_data, self.eac_ctx)

            if self.ca_key:
                ca_pubkey = eac.CA_get_pubkey(self.eac_ctx, ef_card_security_data)
                if 1 != eac.CA_set_key(self.eac_ctx, self.ca_key, ca_pubkey):
                    eac.print_ossl_err()
                    raise SwError(SW["WARN_NOINFO63"])
            else:
                # we don't have a good CA key, so we simply generate an ephemeral one
                comp_pubkey = eac.TA_STEP3_generate_ephemeral_key(self.eac_ctx)
                pubkey = eac.CA_STEP2_get_eph_pubkey(self.eac_ctx)
                if not comp_pubkey or not pubkey:
                    eac.print_ossl_err()
                    raise SwError(SW["WARN_NOINFO63"])

                # save public key in EF.CardSecurity (and invalidate the signature)
                # FIXME this only works for the default EF.CardSecurity.
                # Better use an ASN.1 parser to do this manipulation
                ef_card_security = self.mf.select('fid', 0x011d)
                ef_card_security_data = ef_card_security.data
                ef_card_security_data = ef_card_security_data[:61+4+239+2+1] + pubkey + ef_card_security_data[61+4+239+2+1+len(pubkey):]
                ef_card_security.data = ef_card_security_data

        nonce = eac.PACE_STEP1_enc_nonce(self.eac_ctx, self.sec)
        if not nonce:
            eac.print_ossl_err()
            raise SwError(SW["WARN_NOINFO63"])

        resp = nPA_SE.__pack_general_authenticate([[0x80, len(nonce), nonce]])

        self.eac_step += 1

        return 0x9000, resp

    def __eac_pace_step2(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)

        pubkey = eac.PACE_STEP3A_generate_mapping_data(self.eac_ctx)
        if not pubkey:
            eac.print_ossl_err()
            raise SwError(SW["WARN_NOINFO63"])

        for tag, length, value in tlv_data:
            if tag == 0x81:
                eac.PACE_STEP3A_map_generator(self.eac_ctx, value)
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        self.eac_step += 1

        return 0x9000, nPA_SE.__pack_general_authenticate([[0x82, len(pubkey), pubkey]])

    def __eac_pace_step3(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)

        self.my_pace_eph_pubkey = eac.PACE_STEP3B_generate_ephemeral_key(self.eac_ctx)
        if not self.my_pace_eph_pubkey:
            eac.print_ossl_err()
            raise SwError(SW["WARN_NOINFO63"])
        eph_pubkey = self.my_pace_eph_pubkey

        for tag, length, value in tlv_data:
            if tag == 0x83:
                self.pace_opp_pub_key = value
                eac.PACE_STEP3B_compute_shared_secret(self.eac_ctx, self.pace_opp_pub_key)
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        self.eac_step += 1

        return 0x9000, nPA_SE.__pack_general_authenticate([[0x84, len(eph_pubkey), eph_pubkey]])

    def __eac_pace_step4(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)
        eac.PACE_STEP3C_derive_keys(self.eac_ctx)
        my_token = eac.PACE_STEP3D_compute_authentication_token(self.eac_ctx, self.pace_opp_pub_key)
        token = ""
        for tag, length, value in tlv_data:
            if tag == 0x85:
                token = value
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        if not my_token or 1 != eac.PACE_STEP3D_verify_authentication_token(self.eac_ctx, token):
            eac.print_ossl_err()
            raise SwError(SW["WARN_NOINFO63"])

        print "Established PACE channel"

        if self.at.keyref_is_can():
            if (self.sam.counter == 1):
                self.sam.active = True
                print "PIN resumed"
        elif self.at.keyref_is_pin():
            self.sam.active = True
            self.sam.counter = 3
        elif self.at.keyref_is_puk():
            self.sam.active = True
            self.sam.counter = 3
            print "PIN unblocked"

        self.eac_step += 1
        self.at.algorithm = "TA"

        self.new_encryption_ctx = eac.EAC_ID_PACE

        result = [[0x86, len(my_token), my_token]]
        if self.at.chat:
            if self.cvca:
                self.car = CVC(self.cvca).get_chr()
            result.append([0x87, len(self.car), self.car])
            if (self.disable_checks):
                eac.TA_disable_checks(self.eac_ctx)
            if not eac.EAC_CTX_init_ta(self.eac_ctx, None, self.cvca):
                eac.print_ossl_err()
                raise SwError(SW["WARN_NOINFO63"])


        return 0x9000, nPA_SE.__pack_general_authenticate(result)

    def __eac_ca(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)

        pubkey = ""
        for tag, length, value in tlv_data:
            if tag == 0x80:
                pubkey = value
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        if eac.CA_STEP4_compute_shared_secret(self.eac_ctx, pubkey) != 1:
            eac.print_ossl_err()
            raise SwError(SW["ERR_NOINFO69"]) 

        nonce, token = eac.CA_STEP5_derive_keys(self.eac_ctx, pubkey)
        if not nonce or not token:
            eac.print_ossl_err()
            raise SwError(SW["WARN_NOINFO63"])

        self.eac_step += 1

        print "Generated Nonce and Authentication Token for CA"

        # TODO activate SM
        self.new_encryption_ctx = eac.EAC_ID_CA

        return 0x9000, nPA_SE.__pack_general_authenticate([[0x81,
            len(nonce), nonce], [0x82, len(token), token]])

    def verify_certificate(self, p1, p2, data):
        if (p1, p2) != (0x00, 0xbe):
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        cert = bertlv_pack([[0x7f21, len(data), data]])
        if 1 != eac.TA_STEP2_import_certificate(self.eac_ctx, cert):
            eac.print_ossl_err()
            raise SwError(SW["ERR_NOINFO69"]) 

        print "Imported Certificate"

        return ""

    def external_authenticate(self, p1, p2, data):
        """
        Authenticate the terminal to the card. Check whether Terminal correctly
        encrypted the given challenge or not
        """
        if self.dst.keyref_public_key: # TODO check if this is the correct CAR
            id_picc = eac.EAC_Comp(self.eac_ctx, eac.EAC_ID_PACE, self.my_pace_eph_pubkey)

            # FIXME auxiliary_data might be from an older run of PACE
            if hasattr(self.at, "auxiliary_data"):
                auxiliary_data = self.at.auxiliary_data
            else:
                auxiliary_data = None

            if 1 != eac.TA_STEP6_verify(self.eac_ctx, self.at.iv, id_picc,
                    auxiliary_data, data):
                eac.print_ossl_err()
                raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

            print "Terminal's signature verified"

            self.eac_step += 1

            return 0x9000, ""

        raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

    def compute_cryptographic_checksum(self, p1, p2, data):
        checksum = eac.EAC_authenticate(self.eac_ctx, data)
        if not checksum:
            eac.print_ossl_err()
            raise SwError(SW["ERR_NOINFO69"]) 

        return checksum

    def encipher(self, p1, p2, data):
        padded = vsCrypto.append_padding(self.cct.blocklength, data)
        cipher = eac.EAC_encrypt(self.eac_ctx, padded)
        if not cipher:
            eac.print_ossl_err()
            raise SwError(SW["ERR_NOINFO69"]) 

        return cipher

    def decipher(self, p1, p2, data):
        plain = eac.EAC_decrypt(self.eac_ctx, data)
        if not plain:
            eac.print_ossl_err()
            raise SwError(SW["ERR_NOINFO69"]) 

        return plain

    def protect_response(self, sw, result):
        """
        This method protects a response APDU using secure messaging mechanisms
        
        :returns: the protected data and the SW bytes
        """

        return_data = ""

        if result != "":
            # Encrypt the data included in the RAPDU
            encrypted = self.encipher(0x82, 0x80, result)
            encrypted = "\x01" + encrypted
            encrypted_tlv = bertlv_pack([(
                                SM_Class["CRYPTOGRAM_PADDING_INDICATOR_ODD"],
                                len(encrypted),
                                encrypted)])
            return_data += encrypted_tlv 

        sw_str = inttostring(sw)
        length = len(sw_str) 
        tag = SM_Class["PLAIN_PROCESSING_STATUS"]
        tlv_sw = bertlv_pack([(tag, length, sw_str)])
        return_data += tlv_sw
        
        if self.cct.algorithm == None:
            raise SwError(SW["CONDITIONSNOTSATISFIED"])
        elif self.cct.algorithm == "CC":
            tag = SM_Class["CHECKSUM"]
            padded = vsCrypto.append_padding(self.cct.blocklength, return_data)
            auth = self.compute_cryptographic_checksum(0x8E, 0x80, padded)
            length = len(auth)
            return_data += bertlv_pack([(tag, length, auth)])
        elif self.cct.algorithm == "SIGNATURE":
            tag = SM_Class["DIGITAL_SIGNATURE"]
            hash = self.hash(0x90, 0x80, return_data)
            auth = self.compute_digital_signature(0x9E, 0x9A, hash)
            length = len(auth)
            return_data += bertlv_pack([(tag, length, auth)])
        
        return sw, return_data


class nPA_SAM(SAM):

    def __init__(self, pin, can, mrz, puk, mf, default_se = nPA_SE):
        SAM.__init__(self, pin, None, mf)
        self.active = True
        self.current_SE = default_se(self.mf, self)
        self.can = can
        self.mrz = mrz
        self.puk = puk
        self.counter_puk = 10

    def general_authenticate(self, p1, p2, data):
        return self.current_SE.general_authenticate(p1, p2, data)

    def reset_retry_counter(self, p1, p2, data):
        # check if PACE was successful
        if self.current_SE.eac_step < 4:
            raise SwError(SW["ERR_SECSTATUS"]) 

        # TODO check CAN and PIN for the correct character set
        if p1 == 0x02:
            # change secret
            if p2 == self.current_SE.at.PACE_CAN:
                self.can = data
                print "Changed CAN to %r" % self.can
            elif p2 == self.current_SE.at.PACE_PIN:
                # TODO allow terminals to change the PIN with permission "CAN allowed"
                if not self.current_SE.at.keyref_is_pin():
                    raise SwError(SW["ERR_CONDITIONNOTSATISFIED"]) 
                self.PIN = data
                print "Changed PIN to %r" % self.PIN
            else:
                raise SwError(SW["ERR_DATANOTFOUND"])
        elif p1 == 0x03:
            # resume/unblock secret
            if p2 == self.current_SE.at.PACE_CAN:
                # CAN has no counter
                pass
            elif p2 == self.current_SE.at.PACE_PIN:
                if self.current_SE.at.keyref_is_can():
                    self.active = True
                    print "Resumed PIN"
                elif self.current_SE.at.keyref_is_pin():
                    # PACE was successful with PIN, nothing to do resume/unblock
                    pass
                elif self.current_SE.at.keyref_is_puk():
                    # TODO unblock PIN for signature
                    print "Unblocked PIN"
                    self.active = True
                    self.counter = 3
                else:
                    raise SwError(SW["ERR_CONDITIONNOTSATISFIED"]) 
            else:
                raise SwError(SW["ERR_DATANOTFOUND"])
        else:
            raise SwError(SW["ERR_INCORRECTP1P2"])

        return 0x9000, ""

    def external_authenticate(self, p1, p2, data):
        return self.current_SE.external_authenticate(p1, p2, data)

    def get_challenge(self, p1, p2, data):
        if self.current_SE.eac_step == 4:
            # TA
            if (p1 != 0x00 or p2 != 0x00):
                raise SwError(SW["ERR_INCORRECTP1P2"])
        
            self.last_challenge = eac.TA_STEP4_get_nonce(self.current_SE.eac_ctx)
            if not self.last_challenge:
                eac.print_ossl_err()
                raise SwError(SW["ERR_NOINFO69"])
        else:
            SAM.get_challenge(self, p1, p2, data)

        return SW["NORMAL"], self.last_challenge

    def verify(self, p1, p2, data):
        if (p1 != 0x80 or p2 != 0x00):
            raise SwError(SW["ERR_INCORRECTP1P2"])

        if self.current_SE.eac_step == 6:
            structure = unpack(data)
            for tag, length, value in structure:
                if tag == 6 and ALGO_MAPPING[value] == "DateOfExpiry":
                    # hell yes, this is a valid nPA
                    # TODO actually check it...
                    return SW["NORMAL"], ""
                if tag == 6 and ALGO_MAPPING[value] == "DateOfBirth":
                    # hell yes, we are old enough
                    # TODO actually check it...
                    return SW["NORMAL"], ""
                if tag == 6 and ALGO_MAPPING[value] == "CommunityID":
                    # well OK, we are living there
                    # TODO actually check it...
                    return SW["NORMAL"], ""

        raise SwError(SW["WARN_NOINFO63"])

    def parse_SM_CAPDU(self, CAPDU, header_authentication):
        if hasattr(self.current_SE, "new_encryption_ctx"):
            if self.current_SE.new_encryption_ctx == eac.EAC_ID_PACE:
                protocol = "PACE"
            else:
                protocol = "CA"
            print "switching to new encryption context established in %s:" % protocol
            print eac.EAC_CTX_print_private(self.current_SE.eac_ctx, 4)

            eac.EAC_CTX_set_encryption_ctx(self.current_SE.eac_ctx, self.current_SE.new_encryption_ctx)

            delattr(self.current_SE, "new_encryption_ctx")

        eac.EAC_increment_ssc(self.current_SE.eac_ctx)
        return SAM.parse_SM_CAPDU(self, CAPDU, 1)

    def protect_result(self, sw, unprotected_result):
        eac.EAC_increment_ssc(self.current_SE.eac_ctx)
        return SAM.protect_result(self, sw, unprotected_result)
