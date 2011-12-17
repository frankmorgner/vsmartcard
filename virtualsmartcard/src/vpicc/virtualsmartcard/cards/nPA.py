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
from virtualsmartcard.SEutils import ControlReferenceTemplate
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
        try:
            ControlReferenceTemplate.parse_SE_config(self, config)
        except SwError as e:
            structure = unpack(config)
            for tlv in structure:
                tag, length, value = tlv
                if tag == 0x7f4c:
                    from chat import CHAT
                    chat = CHAT(bertlv_pack([[tag, length, value]]))
                    print(chat)
                elif tag == 0x67:
                    auxiliary_data = value
                elif tag == 0x80 or tag == 0x83 or tag == 0x84:
                    # handled by ControlReferenceTemplate.parse_SE_config
                    pass
                elif tag == 0x91:
                    eph_key = value
                    # TODO handle security environment
                else:
                    raise SwError(SW["ERR_REFNOTUSABLE"])

        return 0x9000 , ""

class nPA_SAM(SAM):
    # TODO move EAC to the securiity environment
    # TODO call __eac_abort whenever an error occurred

    eac_step = make_property("eac_step", "next step to performed for EAC")

    def __init__(self, pin, can, mrz, puk, mf):
        SAM.__init__(self, None, None, mf)
        self.current_SE.at = nPA_AT_CRT() 
        self.eac_step = 0
        self.eac_ctx = None
        self.sec = None
        self.pin = pin
        self.can = can
        self.mrz = mrz
        self.puk = puk

    def general_authenticate(self, p1, p2, data):
        def notImplemented(*argz, **args):
            raise SwError(SW["WARN_NOINFO63"])

        if (p1, p2) != (0x00, 0x00):
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        if (self.eac_step == 0):
            return self.__eac_pace_step1(data)
        elif (self.eac_step == 1):
            return self.__eac_pace_step2(data)
        elif (self.eac_step == 2):
            return self.__eac_pace_step3(data)
        elif (self.eac_step == 3):
            return self.__eac_pace_step4(data)

        raise SwError(SW["ERR_INCORRECTPARAMETERS"])

    def __eac_abort(self):
        pace.EAC_CTX_clear_free(self.eac_ctx)
        pace.PACE_SEC_clear_free(self.sec)

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
        tlv_data = nPA_SAM.__unpack_general_authenticate(data)
        if self.current_SE.at.algorithm != "PACE" or tlv_data != []:
            print 'value %r' % tlv_data
            raise SwError(SW["WARN_NOINFO63"])

        self.__eac_abort()

        self.eac_ctx = pace.EAC_CTX_new()
        if self.current_SE.at.keyref == '\x01':
            self.sec = pace.PACE_SEC_new(self.mrz, pace.PACE_MRZ)
        elif self.current_SE.at.keyref == '\x02':
            self.sec = pace.PACE_SEC_new(self.can, pace.PACE_CAN)
        elif self.current_SE.at.keyref == '\x03':
            self.sec = pace.PACE_SEC_new(self.pin, pace.PACE_PIN)
        elif self.current_SE.at.keyref == '\x04':
            self.sec = pace.PACE_SEC_new(self.puk, pace.PACE_PUK)
        else:
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        ef_card_access = self.mf.select('fid', 0x011c)
        ef_card_access_data = ef_card_access.getenc('data')
        pace.EAC_CTX_init_ef_cardaccess(ef_card_access_data, self.eac_ctx)

        nonce = pace.buf2string(pace.PACE_STEP1_enc_nonce(self.eac_ctx, self.sec))
        resp = nPA_SAM.__pack_general_authenticate([[0x80, len(nonce), nonce]])

        self.eac_step += 1

        return 0x9000, resp

    def __eac_pace_step2(self, data):
        tlv_data = nPA_SAM.__unpack_general_authenticate(data)
        if self.current_SE.at.algorithm != "PACE":
            raise SwError(SW["WARN_NOINFO63"])

        pubkey = pace.buf2string(pace.PACE_STEP3A_generate_mapping_data(self.eac_ctx))

        for tag, length, value in tlv_data:
            if tag == 0x81:
                pace.PACE_STEP3A_map_generator(self.eac_ctx, pace.get_buf(value))
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        self.eac_step += 1

        return 0x9000, nPA_SAM.__pack_general_authenticate([[0x82, len(pubkey), pubkey]])

    def __eac_pace_step3(self, data):
        tlv_data = nPA_SAM.__unpack_general_authenticate(data)
        if self.current_SE.at.algorithm != "PACE":
            raise SwError(SW["WARN_NOINFO63"])

        my_epp_pubkey = pace.buf2string(pace.PACE_STEP3B_generate_ephemeral_key(self.eac_ctx))

        for tag, length, value in tlv_data:
            if tag == 0x83:
                self.pace_opp_pub_key = pace.get_buf(value)
                pace.PACE_STEP3B_compute_shared_secret(self.eac_ctx, self.pace_opp_pub_key)
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        self.eac_step += 1

        return 0x9000, nPA_SAM.__pack_general_authenticate([[0x84, len(my_epp_pubkey), my_epp_pubkey]])

    def __eac_pace_step4(self, data):
        tlv_data = nPA_SAM.__unpack_general_authenticate(data)
        if self.current_SE.at.algorithm != "PACE":
            raise SwError(SW["WARN_NOINFO63"])

        pace.PACE_STEP3C_derive_keys(self.eac_ctx)
        my_token = pace.buf2string(pace.PACE_STEP3D_compute_authentication_token(self.eac_ctx, self.pace_opp_pub_key))

        for tag, length, value in tlv_data:
            if tag == 0x85:
                pace.PACE_STEP3D_verify_authentication_token(self.eac_ctx, pace.get_buf(value))
            else:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        self.eac_step += 1

        # TODO activate SM

        return 0x9000, nPA_SAM.__pack_general_authenticate([[0x86, len(my_token), my_token]])
