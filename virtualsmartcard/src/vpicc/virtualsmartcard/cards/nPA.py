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
from virtualsmartcard.ConstantDefinitions import CRT_TEMPLATE
from virtualsmartcard.TLVutils import unpack, bertlv_pack
from virtualsmartcard.SmartcardFilesystem import make_property
from chat import CHAT
import pace

class nPA_AT_CRT(ControlReferenceTemplate):
    def __init__(self):
        ControlReferenceTemplate.__init__(self, CRT_TEMPLATE["AT"])

    def parse_SE_config(self, config):
        r = 0x9000
        try:
            ControlReferenceTemplate.parse_SE_config(self, config)
        except SwError as e:
            structure = unpack(config)
            for tlv in structure:
                tag, length, value = tlv
                if tag == 0x7f4c:
                    chat = CHAT(bertlv_pack([[tag, length, value]]))
                    print(chat)
                elif tag == 0x67:
                    self.auxiliary_data = value
                elif tag == 0x80 or tag == 0x84 or tag == 0x83:
                    # handled by ControlReferenceTemplate.parse_SE_config
                    pass
                elif tag == 0x91:
                    self.eph_pub_key = value
                else:
                    raise SwError(SW["ERR_REFNOTUSABLE"])

        structure = unpack(config)
        for tlv in structure:
            if [0x83, len('\x03'), '\x03'] == tlv:
                if self.sam.counter <= 0:
                    r = 0x63c0
                elif self.sam.counter == 1:
                    r = 0x63c1
                elif self.sam.counter == 2:
                    r = 0x63c2
        return r, ""

class nPA_SE(Security_Environment):
    # TODO call __eac_abort whenever an error occurred

    def __init__(self, MF, SAM):
        Security_Environment.__init__(self, MF, SAM)
        self.at = nPA_AT_CRT() 
        self.eac_step = 0
        self.sec = None
        self.eac_ctx = None

    def _set_SE(self, p2, data):
        sw, resp = Security_Environment._set_SE(self, p2, data)
        
        if self.at.algorithm == "PACE":
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

        raise SwError(SW["ERR_INCORRECTPARAMETERS"])

    def __eac_abort(self):
        pace.EAC_CTX_clear_free(self.eac_ctx)
        self.eac_ctx = None
        pace.PACE_SEC_clear_free(self.sec)
        self.sec = None

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

        self.__eac_abort()

        self.eac_ctx = pace.EAC_CTX_new()
        if self.at.keyref == '\x01':
            self.sec = pace.PACE_SEC_new(self.sam.mrz, pace.PACE_MRZ)
        elif self.at.keyref == '\x02':
            self.sec = pace.PACE_SEC_new(self.sam.can, pace.PACE_CAN)
        elif self.at.keyref == '\x03':
            if self.sam.counter <= 0:
                print "Must use PUK to unblock"
                raise SwError(SW["WARN_NOINFO63"])
            if self.sam.counter == 1 and not self.sam.active:
                print "Must use CAN to activate"
                return 0x63c1, ""
            self.sec = pace.PACE_SEC_new(self.sam.PIN, pace.PACE_PIN)
            self.sam.counter -= 1
            if self.sam.counter <= 1:
                self.sam.active = False
        elif self.at.keyref == '\x04':
            if self.sam.counter_puk <= 0:
                raise SwError(SW["WARN_NOINFO63"])
            self.sec = pace.PACE_SEC_new(self.sam.puk, pace.PACE_PUK)
            self.sam.counter_puk -= 1
        else:
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        ef_card_access = self.mf.select('fid', 0x011c)
        ef_card_access_data = ef_card_access.getenc('data')
        pace.EAC_CTX_init_ef_cardaccess(ef_card_access_data, self.eac_ctx)

        nonce = pace.buf2string(pace.PACE_STEP1_enc_nonce(self.eac_ctx, self.sec))
        resp = nPA_SE.__pack_general_authenticate([[0x80, len(nonce), nonce]])

        self.eac_step += 1

        return 0x9000, resp

    def __eac_pace_step2(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)

        pubkey = pace.buf2string(pace.PACE_STEP3A_generate_mapping_data(self.eac_ctx))

        for tag, length, value in tlv_data:
            if tag == 0x81:
                pace.PACE_STEP3A_map_generator(self.eac_ctx, pace.get_buf(value))
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        self.eac_step += 1

        return 0x9000, nPA_SE.__pack_general_authenticate([[0x82, len(pubkey), pubkey]])

    def __eac_pace_step3(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)

        my_epp_pubkey = pace.buf2string(pace.PACE_STEP3B_generate_ephemeral_key(self.eac_ctx))

        for tag, length, value in tlv_data:
            if tag == 0x83:
                self.pace_opp_pub_key = pace.get_buf(value)
                pace.PACE_STEP3B_compute_shared_secret(self.eac_ctx, self.pace_opp_pub_key)
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        self.eac_step += 1

        return 0x9000, nPA_SE.__pack_general_authenticate([[0x84, len(my_epp_pubkey), my_epp_pubkey]])

    def __eac_pace_step4(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)
        pace.PACE_STEP3C_derive_keys(self.eac_ctx)
        my_token = pace.buf2string(pace.PACE_STEP3D_compute_authentication_token(self.eac_ctx, self.pace_opp_pub_key))
        token = ""
        for tag, length, value in tlv_data:
            if tag == 0x85:
                token = value
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        if 1 != pace.PACE_STEP3D_verify_authentication_token(self.eac_ctx, pace.get_buf(token)):
            raise SwError(SW["WARN_NOINFO63"])

        if self.at.keyref == '\x02':
            if (self.sam.counter == 1):
                self.sam.active = True
                print "PIN resumed"
        elif self.at.keyref == '\x04':
            self.sam.active = True
            self.sam.counter = 3
            print "PIN unblocked"
        elif self.at.keyref == '\x03':
            self.sam.active = True
            self.sam.counter = 3

        self.eac_step += 1

        # TODO activate SM

        return 0x9000, nPA_SE.__pack_general_authenticate([[0x86, len(my_token), my_token]])

    def __eac_ca(self, data):
        tlv_data = nPA_SE.__unpack_general_authenticate(data)

        pubkey = ""
        for tag, length, value in tlv_data:
            if tag == 0x80:
                pubkey = value
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        pace.CA_STEP4_compute_shared_secret(self.eac_ctx,
                pace.get_buf(pubkey))

        nonce, token = pace.CA_STEP5_derive_keys(self.eac_ctx,
                pace.get_buf(pubkey))

        # TODO activate SM

        return 0x9000, nPA_SE.__pack_general_authenticate([[0x81,
            len(nonce), nonce], [0x82, len(token), token]])

    def verify_certificate(se, p1, p2, data):
        if (p1, p2) != (0x00, 0xbe):
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        cert = bertlv_pack([[0x7f, len(data), data]])
        if 1 != pace.TA_STEP2_import_certificate(self.eac_ctx, pace.get_buf(cert)):
            raise SwError(SW["ERR_NOINFO69"]) 

    def external_authenticate(self, p1, p2, data):
        """
        Authenticate the terminal to the card. Check whether Terminal correctly
        encrypted the given challenge or not
        """
        if self.at.algorithm == "TA":
            if self.last_challenge is None:
                raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

            pace.EAC_CTX_init_ta(self.eac_ctx, "", 0, self.dst.keyref,
                    len(self.dst.keyref), "", 0)

            if 1 != pace.TA_STEP6_verify(self.eac_ctx,
                    pace.get_buf(self.at.eph_pub_key), id_picc,
                    pace.get_buf(self.last_challenge),
                    pace.get_buf(self.auxiliary_data), pace.get_buf(data)):
                raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

            self.eac_step += 1

            return 0x9000, ""

        raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])


class nPA_SAM(SAM):

    eac_step = make_property("eac_step", "next step to performed for EAC")

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
