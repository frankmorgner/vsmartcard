#!/usr/bin/python3

# Generates a PTEID SmartCard
# Author: João Paulo Barraca

from cryptography import x509
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID
import json
from binascii import b2a_base64
import datetime
import uuid


def generate_ec(subject, duration=365 * 15, issuer=None, signing_key=None):

    one_day = datetime.timedelta(1, 0, 0)
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=4096,
        backend=default_backend()
    )

    public_key = private_key.public_key()
    builder = x509.CertificateBuilder()
    builder = builder.subject_name(subject)

    if issuer is None:
        issuer = subject

    builder = builder.issuer_name(issuer)

    builder = builder.not_valid_before(
        datetime.datetime.today() - datetime.timedelta(1, 0, 0))
    builder = builder.not_valid_after(
        datetime.datetime.today() + datetime.timedelta(duration, 0, 0))
    builder = builder.serial_number(int(uuid.uuid4()))
    builder = builder.public_key(public_key)
    builder = builder.add_extension(
        x509.BasicConstraints(ca=True, path_length=None), critical=True,
    )

    builder = builder.add_extension(
        x509.KeyUsage(
            key_cert_sign=True,
            crl_sign=True,
            digital_signature=False,
            content_commitment=False,
            key_encipherment=False,
            data_encipherment=False,
            key_agreement=False,
            encipher_only=False,
            decipher_only=False),
        critical=False
    )

    if signing_key is None:
        signing_key = private_key

    certificate = builder.sign(
        private_key=signing_key, algorithm=hashes.SHA256(),
        backend=default_backend()
    )

    return certificate, private_key


def generate_user(subject, duration=365 * 15, issuer=None, signing_key=None,
                  digital_signature=False, key_agreement=False,
                  content_commitment=True):
    one_day = datetime.timedelta(1, 0, 0)
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=3072,
        backend=default_backend()
    )

    public_key = private_key.public_key()
    builder = x509.CertificateBuilder()
    builder = builder.subject_name(subject)

    if issuer is None:
        issuer = subject

    builder = builder.issuer_name(subject)

    builder = builder.not_valid_before(
        datetime.datetime.today() - datetime.timedelta(1, 0, 0))
    builder = builder.not_valid_after(
        datetime.datetime.today() + datetime.timedelta(duration, 0, 0))
    builder = builder.serial_number(int(uuid.uuid4()))
    builder = builder.public_key(public_key)
    builder = builder.add_extension(
        x509.BasicConstraints(ca=False, path_length=None), critical=False,
    )

    builder = builder.add_extension(
        x509.KeyUsage(
            key_cert_sign=False,
            crl_sign=False,
            digital_signature=digital_signature,
            content_commitment=content_commitment,
            key_encipherment=False,
            data_encipherment=False,
            key_agreement=key_agreement,
            encipher_only=False,
            decipher_only=False),
        critical=False
    )

    if signing_key is None:
        signing_key = private_key

    certificate = builder.sign(
        private_key=signing_key, algorithm=hashes.SHA256(),
        backend=default_backend()
    )

    return certificate, private_key

def load_x509_certificate(fname):
    try:
        return x509.load_pem_x509_certificate(open(fname + ".pem", 'rb').read())
    except:
        pass
    return None

def load_private_key(fname):
    try:
        return serialization.load_pem_private_key(open(fname + ".key", 'rb').read())
    except:
        pass
    return None

def dump_certificate(fname, cert):
    with open(fname + '.pem', 'wb') as f:
        f.write(cert.public_bytes(serialization.Encoding.PEM))

def dump_private_key(fname, pkey):
    with open(fname + '.key', 'wb') as f:
        data = pkey.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.TraditionalOpenSSL,
                encryption_algorithm=serialization.NoEncryption()
                )

        f.write(data)

def dump_cert_key(fname, cert, private_key):
    dump_certificate(fname, cert)
    dump_private_key(fname, private_key)

def load_cert_key(fname):
    cert = load_x509_certificate(fname)
    key = load_private_key(fname)
    return cert, key

##### Starts here

force_create = False

ec_raizestado, ec_raizestado_private_key = load_cert_key('ec_raizestado')

if ec_raizestado is None or ec_raizestado_private_key is None:
    print("Creating EC Raiz Estado")
    force_create = True
    subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, u'ECRaizEstado VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME,
                           u'Sistema de Certificação Electrónica do Estado VIRTUAL'),
        x509.NameAttribute(NameOID.COUNTRY_NAME, u'PT')
    ])

    ec_raizestado, ec_raizestado_private_key = generate_ec(subject, duration=365 * 25)
    dump_cert_key('ec_raizestado', ec_raizestado, ec_raizestado_private_key)

    
ec_cc, ecc_private_key = load_cert_key('ec_cc') if not force_create else (None, None)

if ec_cc is None or ec_cc_private_key is None:
    print("Creating EC CC")
    force_create = True
    subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, u'Cartão de Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME,
                           u'SCEE - Sistema de Certificação Electrónica do Estado VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'ECEstado VIRTUAL'),
        x509.NameAttribute(NameOID.COUNTRY_NAME, u'PT')
    ])

    ec_cc, ec_cc_private_key = generate_ec(
        subject, duration=365 * 15, issuer=ec_raizestado.subject, signing_key=ec_raizestado_private_key)

    dump_cert_key('ec_cc', ec_raizestado, ec_raizestado_private_key)



ec_auth, ec_auth_private_key = load_cert_key('ec_auth') if not force_create else (None, None)

if ec_auth is None or ec_auth_private_key is None:
    print("Creating EC Auth")
    force_create = True
    subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME,
                           u'EC de Autenticação do Cartão de Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME,
                           u'Instituto dos Registos e do Notariado I.P. VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'subECEstado VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'Cartão de Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.COUNTRY_NAME, u'PT')
    ])
    ec_auth, ec_auth_private_key = generate_ec(
        subject, duration=365 * 12, issuer=ec_cc.subject, signing_key=ec_cc_private_key)

    dump_cert_key('ec_auth', ec_auth, ec_auth_private_key)

user_auth, user_auth_private_key = load_cert_key('user_auth') if not force_create else (None, None)

if user_auth is None or user_auth_private_key is None:
    print("Creating User Auth")
    force_create = True
    subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, u'PAULO VIRTUAL'),
        x509.NameAttribute(NameOID.SERIAL_NUMBER, u'BI123123123'),
        x509.NameAttribute(NameOID.SURNAME, u'VIRTUAL'),
        x509.NameAttribute(NameOID.GIVEN_NAME, u'PAULO'),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME,
                           u'Cartão de Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'Cidadão Português VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'Autentidação do Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.COUNTRY_NAME, u'PT')
    ])

    user_auth, user_auth_private_key = generate_user(
        subject, duration=365 * 10, issuer=ec_auth.subject, signing_key=ec_auth_private_key, digital_signature=True, key_agreement=True)
    dump_cert_key('user_auth', user_auth, user_auth_private_key)



ec_sign, ec_sign_private_key = load_cert_key('ec_sign') if not force_create else (None, None)

if ec_sign is None or ec_sign_private_key is None:
    print("Creating EC Sign")
    force_create = True
    subject = x509.Name([
        x509.NameAttribute(
            NameOID.COMMON_NAME, u'EC de Assinatura Digital Qualificada do Cartão de Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME,
                           u'Instituto dos Registos e do Notariado I.P. VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'subECEstado VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'Cartão de Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.COUNTRY_NAME, u'PT')
    ])
    ec_sign, ec_sign_private_key = generate_ec(subject, duration=365 * 12, issuer=ec_cc.subject, signing_key=ec_cc_private_key)

    dump_cert_key('ec_sign', ec_sign, ec_sign_private_key)

user_sign, user_sign_private_key = load_cert_key('user_sign') if not force_create else (None, None)

if user_sign is None or user_sign_private_key is None:
    print("Creating User Sign")
    force_create = True
    subject = x509.Name([
        x509.NameAttribute(NameOID.COMMON_NAME, u'PAULO VIRTUAL'),
        x509.NameAttribute(NameOID.SERIAL_NUMBER, u'BI123123123'),
        x509.NameAttribute(NameOID.SURNAME, u'VIRTUAL'),
        x509.NameAttribute(NameOID.GIVEN_NAME, u'PAULO'),
        x509.NameAttribute(NameOID.ORGANIZATION_NAME,
                           u'Cartão de Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'Cidadão Português VIRTUAL'),
        x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME,
                           u'Assinatura Qualificada do Cidadão VIRTUAL'),
        x509.NameAttribute(NameOID.COUNTRY_NAME, u'PT')
    ])

    user_sign, user_sign_private_key = generate_user(
        subject, duration=365 * 10, issuer=ec_sign.subject, signing_key=ec_sign_private_key, content_commitment=True)

    dump_cert_key('user_sign', user_sign, user_sign_private_key)


print("Creating Card Filesystem")

data = {}

# Files have different security attributes: tag 8C. Using RAW FCI now.
# TODO: Use proper flags for security attributes
# TODO: SOD and other objects are copied from devel cards and need to be created from scratch

# 3F00
data['3f00'] = {'name': b2a_base64(b"\x60\x46\x32\xff\x00\x00\x02") }

data['3f00-0003'] = {'data': 'AAAgBAEB',
                     'fci': b2a_base64(b'\x8C\x05\x1B\x14\x00\x14\x00')}

#  2F00 EF.DIR
"""
   0:d=0  hl=2 l=  29 cons: appl [ 1 ]
   2:d=1  hl=2 l=   9 prim: appl [ 15 ]
  13:d=1  hl=2 l=  12 prim: appl [ 16 ]
  27:d=1  hl=2 l=   2 prim: appl [ 17 ]
  31:d=0  hl=2 l=   0 prim: EOC
"""
data['3f00-2f00'] = {'data': 'YR1PCURGIGlzc3VlclAMUE9SVFVHQUwgRUlEUQJPAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
                     'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#   4F00 - ADF CIA
data['3f00-4f00'] = {'data': '', 
                     'name': b2a_base64(b'\x44\x46\x20\x69\x73\x73\x75\x65\x72'),
                     'fci': b2a_base64(b'\x8C\x03\x06\x00\x00')}

#   5031 - EF OD - Defines 4 CIO EFs: PuKD, PrKD, AOD, 5F004401
"""
    0:d=0  hl=2 l=  10 cons: cont [ 0 ]  # PRIVATE KEYS
    2:d=1  hl=2 l=   8 cons: SEQUENCE
    4:d=2  hl=2 l=   6 prim: OCTET STRING
      0000 - 3f 00 5f 00 ef 0d                                 ?._...
   12:d=0  hl=2 l=  10 cons: cont [ 1 ]  # PUBLIC KEYS
   14:d=1  hl=2 l=   8 cons: SEQUENCE
   16:d=2  hl=2 l=   6 prim: OCTET STRING
      0000 - 3f 00 5f 00 ef 0e                                 ?._...
   24:d=0  hl=2 l=  10 cons: cont [ 4 ]  # CERTIFICATES
   26:d=1  hl=2 l=   8 cons: SEQUENCE
   28:d=2  hl=2 l=   6 prim: OCTET STRING
      0000 - 3f 00 5f 00 ef 0c                                 ?._...
   36:d=0  hl=2 l=  10 cons: cont [ 8 ]  # AUTH OBJECTS
   38:d=1  hl=2 l=   8 cons: SEQUENCE
   40:d=2  hl=2 l=   6 prim: OCTET STRING
      0000 - 3f 00 5f 00 44 01                                 ?._.D.
   48:d=0  hl=2 l=   0 prim: EOC
"""
data['3f00-4f00-5031'] = {'data': 'oAowCAQGPwBfAO8NoQowCAQGPwBfAO8OpAowCAQGPwBfAO8MqAowCAQGPwBfAEQBAAAAAAAAAAAAAAAA',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#   5032 - EF CIA
"""
    0:d=0  hl=2 l=  44 cons: SEQUENCE
    2:d=1  hl=2 l=   1 prim: INTEGER           :01
    5:d=1  hl=2 l=   8 prim: OCTET STRING
      0000 - 99 99 00 00 00 03 55 39-                          ......U9
   15:d=1  hl=2 l=   7 prim: UTF8STRING        :GEMALTO
   24:d=1  hl=2 l=  17 prim: cont [ 0 ]
   43:d=1  hl=2 l=   1 prim: BIT STRING
      0000 - 00                                                .
   46:d=0  hl=2 l=   0 prim: EOC
"""
data['3f00-4f00-5032'] = {'data': 'MCwCAQEECAAABQg5OVZZDAdHRU1BTFRPgBFDQVJUQU8gREUgQ0lEQURBTwMBAAAA',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#   5F00 ADF PKI
data['3f00-5f00'] = {'name': b2a_base64(b'\x44\x46\x20\x69\x73\x73\x75\x65\x73'),
                     'fci': b2a_base64(b'\x8C\x03\x06\x00\x00')}

#   4401 - EF AOD - AUTHENTICATION OBJECTS
"""
    4:d=2  hl=2 l=  21 prim: UTF8STRING        :PIN da Autenticação
   64:d=2  hl=2 l=  17 prim: UTF8STRING        :PIN da Assinatura
  120:d=2  hl=2 l=  13 prim: UTF8STRING        :PIN da Morada
"""
data['3f00-5f00-4401'] = {'data': 'MDowFwwVUElOIGRhIEF1dGVudGljYcOnw6NvMAcEAQECAgCBoRYwFAMCA4gKAQECAQQCAQiAAgCBBAH/MDYwEwwRUElOIGRhIEFzc2luYXR1cmEwBwQBAgICAIKhFjAUAwIDiAoBAQIBBAIBCIACAIIEAf8wMjAPDA1QSU4gZGEgTW9yYWRhMAcEAQMCAgCDoRYwFAMCA4gKAQECAQQCAQiAAgCDBAH/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}


#   EF0C - CertID - CERTIFICATES
"""
    0:d=0  hl=2 l=  57 cons: SEQUENCE
    2:d=1  hl=2 l=  31 cons: SEQUENCE
    4:d=2  hl=2 l=  26 prim: UTF8STRING        :CITIZEN AUTHENTICATION KEY
   32:d=2  hl=2 l=   1 prim: OCTET STRING
      0000 - 01                                                .
   35:d=1  hl=2 l=  10 cons: SEQUENCE
   37:d=2  hl=2 l=   1 prim: OCTET STRING      :E
   40:d=2  hl=2 l=   2 prim: BIT STRING
      0000 - 05 20                                             .
   44:d=2  hl=2 l=   1 prim: INTEGER           :02
   47:d=1  hl=2 l=  10 cons: cont [ 1 ]
   49:d=2  hl=2 l=   8 cons: SEQUENCE
   51:d=3  hl=2 l=   2 cons: SEQUENCE
   53:d=4  hl=2 l=   0 prim: OCTET STRING
   55:d=3  hl=2 l=   2 prim: INTEGER           :0400
   59:d=0  hl=2 l=  56 cons: SEQUENCE
   61:d=1  hl=2 l=  29 cons: SEQUENCE
   63:d=2  hl=2 l=  21 prim: UTF8STRING        :CITIZEN SIGNATURE KEY
   86:d=2  hl=2 l=   1 prim: OCTET STRING
      0000 - 02                                                .
   89:d=2  hl=2 l=   1 prim: INTEGER           :01
   92:d=1  hl=2 l=  11 cons: SEQUENCE
   94:d=2  hl=2 l=   1 prim: OCTET STRING      :F
   97:d=2  hl=2 l=   3 prim: BIT STRING
      0000 - 06 00 40                                          ..@
  102:d=2  hl=2 l=   1 prim: INTEGER           :01
  105:d=1  hl=2 l=  10 cons: cont [ 1 ]
  107:d=2  hl=2 l=   8 cons: SEQUENCE
  109:d=3  hl=2 l=   2 cons: SEQUENCE
  111:d=4  hl=2 l=   0 prim: OCTET STRING
  113:d=3  hl=2 l=   2 prim: INTEGER           :0400
  117:d=0  hl=2 l=   0 prim: EOC
"""
data['3f00-5f00-ef0c'] = {'data': 'MDkwJAwiQ0lUSVpFTiBBVVRIRU5USUNBVElPTiBDRVJUSUZJQ0FURTADBAFFoQwwCjAIBAY/AF8A7wkwNDAfDB1DSVRJWkVOIFNJR05BVFVSRSBDRVJUSUZJQ0FURTADBAFGoQwwCjAIBAY/AF8A7wgwJzASDBBTSUdOQVRVUkUgU1VCIENBMAMEAVGhDDAKMAgEBj8AXwDvDzAsMBcMFUFVVEhFTlRJQ0FUSU9OIFNVQiBDQTADBAFSoQwwCjAIBAY/AF8A7xAwHjAJDAdST09UIENBMAMEAVChDDAKMAgEBj8AXwDvEQ==',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#   EF12 - CertUserD
data['3f00-5f00-ef12'] = {'data': 'MEYwMQwiQ2l0aXplbiBBdXRoZW50aWNhdGlvbiBDZXJ0aWZpY2F0ZQMBADAIMAYDAgeABQAwAwQBRaEMMAowCAQGPwBfAO8JMEEwLAwdQ2l0aXplbiBTaWduYXR1cmUgQ2VydGlmaWNhdGUDAQAwCDAGAwIHgAUAMAMEAUahDDAKMAgEBj8AXwDvCA==',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#   EF0D - PrKD
data['3f00-5f00-ef0d'] = {'data': 'MDkwHwwaQ0lUSVpFTiBBVVRIRU5USUNBVElPTiBLRVkEAQEwCgQBRQMCBSACAQKhCjAIMAIEAAICDAAwODAdDBVDSVRJWkVOIFNJR05BVFVSRSBLRVkEAQICAQEwCwQBRgMDBgBAAgEBoQowCDACBAACAgwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#   EF0E - PuKD
data['3f00-5f00-ef0e'] = {'data': 'MEwwLgwaQ2l0aXplbiBBdXRoZW50aWNhdGlvbiBLZXkDAgeABAGBMAkwBwMCBSAEAYEwDgQBRQMCAkQDAgO4AgECoQowCDACBAACAgwAMEgwKQwVQ2l0aXplbiBTaWduYXR1cmUgS2V5AwIHgAQBgjAJMAcDAgUgBAGCMA8EAUYDAwYgQAMCA7gCAQGhCjAIMAIEAAICDAA=',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#       EF02 - ID
data['3f00-5f00-ef02'] = { 'data': data['3f00-4f00-5032']['data'],
                                'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#       EF05 - Addr
data['3f00-5f00-ef05'] = {'data': 'TgBQVAAAMTEAAExpc2JvYQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAxMTA2AAAAAExpc2JvYQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAxMTA2MjMAAAAAAABOb3NzYSBTZW5ob3JhIGRlIEbDoXRpbWEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQVYAAAAAAAAAAAAAAAAAAAAAAABBdmVuaWRhAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANSBkZSBPdXR1YnJvAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAyMDIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAExpc2JvYQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAxMDUwAAAAADA2NQAAAExJU0JPQQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMjAwODAxAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
                                'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#       EF06 - SOD
data['3f00-5f00-ef06'] = {'data': data['3f00-4f00-5032']['data'],
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#       EF07 - Perso Data
data["3f00-5f00-ef07"] = {'data': b2a_base64(bytes([0] * 1000)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#       EF09 - Cert Auth
data["3f00-5f00-ef09"] = {'data': b2a_base64(user_auth.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

data["auth-private-key"] = {'data': b2a_base64(user_auth_private_key.private_bytes(encoding=serialization.Encoding.DER,
                                    format=serialization.PrivateFormat.TraditionalOpenSSL,
                                    encryption_algorithm=serialization.NoEncryption()))}

#       EF08 - Cert Sign
data["3f00-5f00-ef08"] = {'data': b2a_base64(user_sign.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

data["sign-private-key"] = {'data': b2a_base64(user_sign_private_key.private_bytes(encoding=serialization.Encoding.DER,
                format=serialization.PrivateFormat.TraditionalOpenSSL,
                encryption_algorithm=serialization.NoEncryption()))}

#       EF11 - Cert Root
data["3f00-5f00-ef11"] = {'data': b2a_base64(ec_raizestado.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}


#       EF10 - Cert Root Auth
data["3f00-5f00-ef10"] = {'data': b2a_base64(ec_auth.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}

#       EF0F - Cert Root Sign
data["3f00-5f00-ef0f"] = {'data': b2a_base64(ec_sign.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\x00\xFF\x00')}


print("Creating Final Card Object")
for k in data:
    for kk in data[k]:
        if isinstance(data[k][kk], bytes):
            data[k][kk] = data[k][kk].decode().strip()

with open('card.json', 'w') as f:
    content = json.dumps(data, indent=4, default=str)
    f.write(content)
    f.close()

print("Card Generated to card.json")
