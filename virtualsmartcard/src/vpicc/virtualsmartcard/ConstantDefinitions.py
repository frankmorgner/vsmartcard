#
# Copyright (C) 2011 Frank Morgner, Dominik Oepen
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

MAX_SHORT_LE = 0xff+1
MAX_EXTENDED_LE = 0xffff+1

# Life cycle status byte
LCB = {}
LCB["NOINFORMATION"] = 0x00
LCB["CREATION"] = 0x01
LCB["INITIALISATION"] = 0x03
LCB["ACTIVATED"] = 0x05
LCB["DEACTIVATED"] = 0x04
LCB["TERMINATION"] = 0x0C

# Channel security attribute
CS = {}
CS["NOTSHAREABLE"] = 0x01
CS["SECURED"] = 0x02
CS["USERAUTHENTICATED"] = 0x03

# Security attribute
# Security attribute, Access mode byte for DFs/EFs
SA = {}
SA["AM_DF_DELETESELF"] = SA["AM_EF_DELETEFILE"] = 0x40
SA["AM_DF_TERMINATEDF"] = SA["AM_EF_TERMINATEFILE"] = 0x20
SA["AM_DF_ACTIVATEFILE"] = SA["AM_EF_ACTIVATEFILE"] = 0x10
SA["AM_DF_DEACTIVATEFILE"] = SA["AM_EF_DEACTIVATEFILE"] = 0x08
SA["AM_DF_CREATEDF"] = SA["AM_EF_WRITEBINARY"] = 0x04
SA["AM_DF_CREATEEF"] = SA["AM_EF_UPDATEBINARY"] = 0x02
SA["AM_DF_DELETECHILD"] = SA["AM_EF_READBINARY"] = 0x01

# Security attribute in compact format, Security condition byte
SA["CF_SC_NOCONDITION"] = 0x00
SA["CF_SC_NEVER"] = 0xFF
SA["__CF_SC_ATLEASTONE"] = 0x80
SA["__CF_SC_ALLCONDITIONS"] = 0x00
SA["__CF_SC_SECUREMESSAGING"] = 0x40
SA["__CF_SC_EXTERNALAUTHENTICATION"] = 0x40
SA["__CF_SC_USERAUTHENTICATION"] = 0x40
SA["CF_SC_ONE_SECUREMESSAGING"] = SA["__CF_SC_ATLEASTONE"] | \
    SA["__CF_SC_SECUREMESSAGING"]
SA["CF_SC_ONE_EXTERNALAUTHENTICATION"] = SA["__CF_SC_ATLEASTONE"] | \
    SA["__CF_SC_EXTERNALAUTHENTICATION"]
SA["CF_SC_ONE_USERAUTHENTICATION"] = SA["__CF_SC_ATLEASTONE"] | \
    SA["__CF_SC_USERAUTHENTICATION"]
SA["CF_SC_ALL_SECUREMESSAGING"] = SA["__CF_SC_ALLCONDITIONS"] | \
    SA["__CF_SC_SECUREMESSAGING"]
SA["CF_SC_ALL_EXTERNALAUTHENTICATION"] = SA["__CF_SC_ALLCONDITIONS"] | \
    SA["__CF_SC_EXTERNALAUTHENTICATION"]
SA["CF_SC_ALL_USERAUTHENTICATION"] = SA["__CF_SC_ALLCONDITIONS"] | \
    SA["__CF_SC_USERAUTHENTICATION"]


# Data coding byte
DCB = {}
DCB["ONETIMEWRITE"] = 0x00
DCB["PROPRIETARY"] = 0x20     # we use it for XOR
DCB["WRITEOR"] = 0x40
DCB["WRITEAND"] = 0x60
DCB["BERTLV_FFVALID"] = 0x10
DCB["BERTLV_FFINVALID"] = 0x10

# File descriptor byte
FDB = {}
FDB["NOTSHAREABLEFILE"] = 0x00
FDB["SHAREABLEFILE"] = 0x40
FDB["WORKINGEF"] = 0x00
FDB["INTERNALEF"] = 0x80
FDB["DF"] = 0x38
FDB["EFSTRUCTURE_NOINFORMATIONGIVEN"] = 0x00
FDB["EFSTRUCTURE_TRANSPARENT"] = 0x01
FDB["EFSTRUCTURE_LINEAR_FIXED_NOFURTHERINFO"] = 0x02
FDB["EFSTRUCTURE_LINEAR_FIXED_SIMPLETLV"] = 0x03
FDB["EFSTRUCTURE_LINEAR_VARIABLE_NOFURTHERINFO"] = 0x04
FDB["EFSTRUCTURE_LINEAR_VARIABLESIMPLETLV"] = 0x05
FDB["EFSTRUCTURE_CYCLIC_NOFURTHERINFO"] = 0x06
FDB["EFSTRUCTURE_CYCLIC_SIMPLETLV"] = 0x07

# File identifier
FID = {}
FID["EFDIR"] = 0x2F00
FID["EFATR"] = 0x2F00
FID["MF"] = 0x3F00
FID["PATHSELECTION"] = 0x3FFF
FID["RESERVED"] = 0x3FFF


# Secure Messaging constants
SM_Class = {}

# Basic secure messaging objects
# '80', '81' Plain value not encoded in BER-TLV
SM_Class["PLAIN_VALUE_NO_TLV"] = 0x80
SM_Class["PLAIN_VALUE_NO_TLV_ODD"] = 0x81
# '82', '83' Cryptogram (plain value encoded in BER-TLV and including SM data
# objects, i.e. a SM field)
SM_Class["CRYPTOGRAM_PLAIN_TLV_INCLUDING_SM"] = 0x82
SM_Class["CRYPTOGRAM_PLAIN_TLV_INCLUDING_SM_ODD"] = 0x83
# '84', '85' Cryptogram (plain value encoded in BER-TLV, but not including SM
# data objects)
SM_Class["CRYPTOGRAM_PLAIN_TLV_NO_SM"] = 0x84
SM_Class["CRYPTOGRAM_PLAIN_TLV_NO_SM_ODD"] = 0x85
# '86', '87' Padding-content indicator byte followed by cryptogram (plain
# value not encoded in BER-TLV)
SM_Class["CRYPTOGRAM_PADDING_INDICATOR"] = 0x86
SM_Class["CRYPTOGRAM_PADDING_INDICATOR_ODD"] = 0x87
# '89' Command header (CLA INS P1 P2, four bytes)
SM_Class["PLAIN_COMMAND_HEADER"] = 0x89
# '8E' Cryptographic checksum (at least four bytes)
SM_Class["CHECKSUM"] = 0x8E
# '90', '91' Hash-code
SM_Class["HASH_CODE"] = 0x90
SM_Class["HASH_CODE_ODD"] = 0x91
# '92', '93' Certificate (data not encoded in BER-TLV)
SM_Class["CERTIFICATE"] = 0x92
SM_Class["CERTIFICATE_ODD"] = 0x93
# '94', '95' Security environment identifier (SEID byte)
SM_Class["SECURITY_ENIVRONMENT_ID"] = 0x94
SM_Class["SECURITY_ENIVRONMENT_ID_ODD"] = 0x95
# '96', '97' One or two bytes encoding Ne in the unsecured command-response
# pair (possibly empty)
SM_Class["Ne"] = 0x96
SM_Class["Ne_ODD"] = 0x97
# '99' Processing status (SW1-SW2, two bytes; possibly empty)
SM_Class["PLAIN_PROCESSING_STATUS"] = 0x99
# '9A', '9B' Input data element for the computation of a digital signature
# (the value field is signed)
SM_Class["INPUT_DATA_SIGNATURE_COMPUTATION"] = 0x9A
SM_Class["INPUT_DATA_SIGNATURE_COMPUTATION_ODD"] = 0x9B
# '9C', '9D' Public key
SM_Class["PUBLIC_KEY"] = 0x9C
SM_Class["PUBLIC_KEY_ODD"] = 0x9D
# '9E' Digital signature
SM_Class["DIGITAL_SIGNATURE"] = 0x9E

# Auxiliary secure messaging objects
# Input template for the computation of a hash-code (the template is hashed)
SM_Class["HASH_COMPUTATION_TEMPLATE"] = 0xA0
SM_Class["HASH_COMPUTATION_TEMPLATE_ODD"] = 0xA1
# Input template for the verification of a cryptographic checksum (the
# template is included)
SM_Class["CHECKSUM_VERIFICATION_TEMPLATE"] = 0xA2
# Control reference template for authentication (AT)
SM_Class["AUTH_CRT"] = 0xA4
SM_Class["AUTH_CRT_ODD"] = 0xA5
# Control reference template for key agreement (KAT)
SM_Class["KEY_AGREEMENT_CRT"] = 0xA6
SM_Class["KEY_AGREEMENT_CRT_ODD"] = 0xA7
# Input template for the verification of a digital signature (the template is
# signed)
SM_Class[0xA8] = "SIGNATURE_VERIFICATION_TEMPLATE"
# Control reference template for hash-code (HT)
SM_Class["HASH_CRT"] = 0xAA
SM_Class["HASH_CRT_ODD"] = 0xAB
# Input template for the computation of a digital signature (the concatenated
# value fields are signed)
SM_Class["SIGNATURE_COMPUTATION_TEMPLATE"] = 0xAC
SM_Class["SIGNATURE_COMPUTATION_TEMPLATE_ODD"] = 0xAD
# Input template for the verification of a certificate (the concatenated value
# fields are certified)
SM_Class["CERTIFICATE_VERIFICATION_TEMPLATE"] = 0xAE
SM_Class["CERTIFICATE_VERIFICATION_TEMPLATE_ODD"] = 0xAF
# Plain value encoded in BER-TLV and including SM data objects, i.e. a SM
# field
SM_Class["PLAIN_VALUE_TLV_INCULDING_SM"] = 0xB0
SM_Class["PLAIN_VALUE_TLV_INCULDING_SM_ODD"] = 0xB1
# Plain value encoded in BER-TLV, but not including SM data objects
SM_Class["PLAIN_VALUE_TLV_NO_SM"] = 0xB2
SM_Class["PLAIN_VALUE_TLV_NO_SM_ODD"] = 0xB3
# Control reference template for cryptographic checksum (CCT)
SM_Class["CHECKSUM_CRT"] = 0xB4
SM_Class["CHECKSUM_CRT_ODD"] = 0xB5
# Control reference template for digital signature (DST)
SM_Class["SIGNATURE_CRT"] = 0xB6
SM_Class["SIGNATURE_CRT_ODD"] = 0xB7
# Control reference template for confidentiality (CT)
SM_Class["CONFIDENTIALITY_CRT"] = 0xB8
SM_Class["CONFIDENTIALITY_CRT_ODD"] = 0xB9
# Response descriptor template
SM_Class["RESPONSE_DESCRIPTOR_TEMPLATE"] = 0xBA
SM_Class["RESPONSE_DESCRIPTOR_TEMPLATE_ODD"] = 0xBB
# Input template for the computation of a digital signature (the template is
# signed)
SM_Class["SIGNATURE_COMPUTATION_TEMPLATE"] = 0xBC
SM_Class["SIGNATURE_COMPUTATION_TEMPLATE_ODD"] = 0xBD
# Input template for the verification of a certificate (the template is
# certified)
SM_Class["CERTIFICATE_VERIFICATION_TEMPLATE"] = 0xBE
# }}}

# Tags of control reference templates (CRT)
CRT_TEMPLATE = {}
CRT_TEMPLATE["AT"] = 0xA4
CRT_TEMPLATE["KAT"] = 0xA6
CRT_TEMPLATE["HT"] = 0xAA
CRT_TEMPLATE["CCT"] = 0xB4  # Template for Cryptographic Checksum
CRT_TEMPLATE["DST"] = 0xB6
CRT_TEMPLATE["CT"] = 0xB8  # Template for Confidentiality

# This table maps algorithms to numbers. It is used in the security environment
ALGO_MAPPING = {}
ALGO_MAPPING[b'\x00'] = "DES3-ECB"
ALGO_MAPPING[b'\x01'] = "DES3-CBC"
ALGO_MAPPING[b'\x02'] = "AES-ECB"
ALGO_MAPPING[b'\x03'] = "AES-CBC"
ALGO_MAPPING[b'\x04'] = "DES-ECB"
ALGO_MAPPING[b'\x05'] = "DES-CBC"
ALGO_MAPPING[b'\x06'] = "MD5"
ALGO_MAPPING[b'\x07'] = "SHA"
ALGO_MAPPING[b'\x08'] = "SHA256"
ALGO_MAPPING[b'\x09'] = "MAC"
ALGO_MAPPING[b'\x0a'] = "HMAC"
ALGO_MAPPING[b'\x0b'] = "CC"
ALGO_MAPPING[b'\x0c'] = "RSA"
ALGO_MAPPING[b'\x0d'] = "DSA"

# Recerence control for select command, and record handling commands
REF = {
    "IDENTIFIER_FIRST": 0x00,
    "IDENTIFIER_LAST": 0x01,
    "IDENTIFIER_NEXT": 0x02,
    "IDENTIFIER_PREVIOUS": 0x03,
    "IDENTIFIER_CONTROL": 0x03,
    "NUMBER": 0x04,
    "NUMBER_TO_LAST": 0x05,
    "NUMBER_FROM_LAST": 0x06,
    "NUMBER_CONTROL": 0x07,
    "REFERENCE_CONTROL_RECORD": 0x07,
    "REFERENCE_CONTROL_SELECT": 0x03,
        }
