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
        key_size=1024,
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
        f.write(ec_raizestado.public_bytes(serialization.Encoding.PEM))

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
data['3f00-0001'] = {'name': b2a_base64(b'\x60\x46\x32\xFF\x00\x00\x02')}

data['3f00-0003'] = {'data': 'AAAgBAEB',
                     'fci': b2a_base64(b'\x8C\x05\x1B\x14\xFF\x14\x00')}

data['3f00-2f00'] = {'data': 'YR1PCURGIGlzc3VlclAMUE9SVFVHQUwgRUlEUQJPAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
                     'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#   4F00 - ADF CIA
data['3f00-4f00'] = {'data': '', 'name': b2a_base64(b'\x44\x46\x20\x69\x73\x73\x75\x65\x72'),
                     'fci': b2a_base64(b'\x8C\x03\x06\x00\x00')}

#   5031 - EF OD
data['3f00-4f00-5031'] = {'data': 'oAowCAQGPwBfAO8NoQowCAQGPwBfAO8OpAowCAQGPwBfAO8MqAowCAQGPwBfAEQBAAAAAAAAAAAAAAAA',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#   5032 - EF CIA
data['3f00-4f00-5032'] = {'data': 'MCwCAQEECJmZAAAAA1U5DAdHRU1BTFRPgBFDQVJUQU8gREUgQ0lEQURBTwMBAAAA',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#   5F00 ADF PKI
data['3f00-5f00'] = {'name': b2a_base64(b'\x44\x46\x20\x69\x73\x73\x75\x65\x73'),
                     'fci': b2a_base64(b'\x8C\x03\x06\x00\x00')}
#   4401 - EF AOD
data['3f00-5f00-4401'] = {'data': 'MDowFwwVUElOIGRhIEF1dGVudGljYcOnw6NvMAcEAQECAgCBoRYwFAMCA4gKAQECAQQCAQiAAgCBBAH/MDYwEwwRUElOIGRhIEFzc2luYXR1cmEwBwQBAgICAIKhFjAUAwIDiAoBAQIBBAIBCIACAIIEAf8wMjAPDA1QSU4gZGEgTW9yYWRhMAcEAQMCAgCDoRYwFAMCA4gKAQECAQQCAQiAAgCDBAH/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}
#   EF0C - CertID
data['3f00-5f00-ef0c'] = {'data': 'MDkwJAwiQ0lUSVpFTiBBVVRIRU5USUNBVElPTiBDRVJUSUZJQ0FURTADBAFFoQwwCjAIBAY/AF8A7wkwNDAfDB1DSVRJWkVOIFNJR05BVFVSRSBDRVJUSUZJQ0FURTADBAFGoQwwCjAIBAY/AF8A7wgwJzASDBBTSUdOQVRVUkUgU1VCIENBMAMEAVGhDDAKMAgEBj8AXwDvDzAsMBcMFUFVVEhFTlRJQ0FUSU9OIFNVQiBDQTADBAFSoQwwCjAIBAY/AF8A7xAwHjAJDAdST09UIENBMAMEAVChDDAKMAgEBj8AXwDvEQ==',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#   EF12 - CertUserD
data['3f00-5f00-ef12'] = {'data': 'MEYwMQwiQ2l0aXplbiBBdXRoZW50aWNhdGlvbiBDZXJ0aWZpY2F0ZQMBADAIMAYDAgeABQAwAwQBRaEMMAowCAQGPwBfAO8JMEEwLAwdQ2l0aXplbiBTaWduYXR1cmUgQ2VydGlmaWNhdGUDAQAwCDAGAwIHgAUAMAMEAUahDDAKMAgEBj8AXwDvCA==',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#   EF0D - PrKD
data['3f00-5f00-ef0d'] = {'data': 'MDkwHwwaQ0lUSVpFTiBBVVRIRU5USUNBVElPTiBLRVkEAQEwCgQBRQMCBSACAQKhCjAIMAIEAAICBAAwODAdDBVDSVRJWkVOIFNJR05BVFVSRSBLRVkEAQICAQEwCwQBRgMDBgBAAgEBoQowCDACBAACAgQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#   EF0E - PuKD
data['3f00-5f00-ef0e'] = {'data': 'MEwwLgwaQ2l0aXplbiBBdXRoZW50aWNhdGlvbiBLZXkDAgeABAGBMAkwBwMCBSAEAYEwDgQBRQMCAkQDAgO4AgECoQowCDACBAACAgQAMEgwKQwVQ2l0aXplbiBTaWduYXR1cmUgS2V5AwIHgAQBgjAJMAcDAgUgBAGCMA8EAUYDAwYgQAMCA7gCAQGhCjAIMAIEAAICBAA=',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#       EF02 - ID
data['3f00-5f00-ef02'] = {'data': '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',
                                'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#       EF05 - Addr
data['3f00-5f00-ef05'] = {'data': 'TgBQVAAAMTEAAExpc2JvYQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAxMTA2AAAAAExpc2JvYQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAxMTA2MjMAAAAAAABOb3NzYSBTZW5ob3JhIGRlIEbDoXRpbWEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQVYAAAAAAAAAAAAAAAAAAAAAAABBdmVuaWRhAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANSBkZSBPdXR1YnJvAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAyMDIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAExpc2JvYQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAxMDUwAAAAADA2NQAAAExJU0JPQQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAMjAwODAxAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA',
                                'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#       EF06 - SOD
data['3f00-5f00-ef06'] = {'data': 'd4IJzjCCCcoGCSqGSIb3DQEHAqCCCbswggm3AgEDMQ8wDQYJYIZIAWUDBAIBBQAwgcIGBmeBCAEBAaCBtwSBtDCBsQIBADANBglghkgBZQMEAgEFADCBnDAlAgEBBCDd5/mynI3R1aRbVZgVk+jD1C+AjJNoNKtAjvf9B3x8DzAlAgECBCDJAvR7Sqh5wjLqqgjuP86UHE+yloTQPSvSCCmYdKIBZDAlAgEDBCAh677FKrecKfQ8hT0HB2Q9lwcGKVpkZbHIx3mu4r3e1zAlAgEEBCCcuukEAJ/zIUw4YueB51GssQwODHGuaRghkxeAZAjH76CCBwAwggb8MIIE5KADAgECAghixyCzOBmdrzANBgkqhkiG9w0BAQUFADBVMSQwIgYDVQQDDBsoVGVzdDAyKUNhcnTDo28gZGUgQ2lkYWTDo28xETAPBgNVBAsMCEVDRXN0YWRvMQ0wCwYDVQQKDARTQ0VFMQswCQYDVQQGEwJQVDAeFw0xNDAyMDYxNjAxMTdaFw0xOTA0MjExNjExMTdaMIHaMVMwUQYDVQQDDEooVGVzdGUpIEVudGlkYWRlIENlcnRpZmljYWRvcmEgZGUgRG9jdW1lbnRvcyBkbyBDYXJ0w6NvIGRlIENpZGFkw6NvIDAwMDAxNDEtMCsGA1UECwwkRW50aWRhZGUgQ2VydGlmaWNhZG9yYSBkZSBEb2N1bWVudG9zMSkwJwYDVQQLDCBTZXJ2acOnb3MgZG8gQ2FydMOjbyBkZSBDaWRhZMOjbzEcMBoGA1UECgwTQ2FydMOjbyBkZSBDaWRhZMOjbzELMAkGA1UEBhMCUFQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDSUE99kux+h2/pJpvcgEn0WVTi93bwcpP+AjyCkLlah9zXhvIV7FwsyP60nXbIqM7g1WUUEqxbWoqpHajmoctJF/Tg+eg6ZKQY+grwxcemCAC0gm7NO5dfLzObt/a1hdes1AXff5aOZtjmZwDTx9mUxvAmTcIxPd/Uj/8uONq//PW6TrfVUfQafn3h0bpwnwS/kTHnzBTbwd0oEwyaBn884GHZuh2r/utkVKlml3QRu33+Ae+L94SURFZTlqPWtzuClyYqggfn764Heu1gtaR2J1fA6M6Ey7332XGdNt6DGMmFykAfrugZKQKfGpOX/9CnG2Xr9qklFEsXtt0U2WeVAgMBAAGjggJIMIICRDAMBgNVHRMBAf8EAjAAMA4GA1UdDwEB/wQEAwIHgDAdBgNVHQ4EFgQUwJg6yAe8OGE5Ph75atcK/7a4FGUwHwYDVR0jBBgwFoAUbEvUyUBV2JPc/xE8tabBhJ+l9QkwggEvBgNVHSAEggEmMIIBIjCBsQYLYIRsAQEBAgQAAQUwgaEwgZ4GCCsGAQUFBwICMIGRHoGOAGgAdAB0AHAAOgAvAC8AcABrAGkALgB0AGUAcwB0AGUALgBjAGEAcgB0AGEAbwBkAGUAYwBpAGQAYQBkAGEAbwAuAHAAdAAvAHAAdQBiAGwAaQBjAG8ALwBwAG8AbABpAHQAaQBjAGEAcwAvAHAAYwAvAGMAYwBfAEUAQwBEAF8AcABjAC4AaAB0AG0AbDBsBgpghGwBAQECBAAHMF4wXAYIKwYBBQUHAgEWUGh0dHA6Ly9wa2kudGVzdGUuY2FydGFvZGVjaWRhZGFvLnB0L3B1YmxpY28vcG9saXRpY2FzL2RwYy9jY19lY19jaWRhZGFvX2RwYy5odG1sMF0GA1UdHwRWMFQwUqBQoE6GTGh0dHA6Ly9wa2kudGVzdGUuY2FydGFvZGVjaWRhZGFvLnB0L3B1YmxpY28vbHJjL2NjX2VjX2NpZGFkYW9fY3JsdDAyX2NybC5jcmwwUgYIKwYBBQUHAQEERjBEMEIGCCsGAQUFBzABhjZodHRwOi8vb2NzcC5yb290LnRlc3RlLmNhcnRhb2RlY2lkYWRhby5wdC9wdWJsaWNvL29jc3AwDQYJKoZIhvcNAQEFBQADggIBAHdTkX741I2FGzds0qxVIepKollBNrMGNiumJM+zNRyjdsdnVaZQbYuSEs/vBIZ8ahWNUFugB4WsQSXr05Ee4QD5K7XcUukkAfFJSgv9JNZZw3DYHLfUmAiSWndsjJ2kK4hXwCcqMIBkqpncViN2D54D37VMB8LfPaajJwYJd+Vr5PJMmkGNwka+Et7cnuyXjdK+nH/wJZLo6ojAIMzUzw0Zvhjq/H4zzJHV+E2Eq9zJhwTbGn/FUuoNChPGNitzWEi16M1gPR6+TadA67RJskqTyDZyXsSQGX34zg03+l3Ez5i3xUSsXN+btQY6oezPNNi3OjBB7RA9kEhOcPqrlKOZgXRM6c/3IBJSAEoEqqCnrrVGVZsAWLcZgtY+fa1A61xxjOeCZnAZjKTWITbYi9jDvKtkF6FQia96q4bd2i36gDM30cCjF96naCOQqMLIjcrbSp85KiLmJzT7JMqJQ0+4JII2EaRpMa/kKPOWMcWZicPxE1FNf+XMjeLAk8721C5ciciLQTPUEEu216bISGS6Rmt2PlMPWM+rdMIkbqUkIVyuIVNCLFXWV/SuquPHloLfeXdtdBMn80XSV6ADvVtJ1dD/HLH/UV07p04wLFOoH8Z5WhM/Tm8sHorqsxVyctk4F26hvEld5h8r44PEWH5kKVafcUTNwRtN+OM4hRqeMYIB1jCCAdICAQEwYTBVMSQwIgYDVQQDDBsoVGVzdDAyKUNhcnTDo28gZGUgQ2lkYWTDo28xETAPBgNVBAsMCEVDRXN0YWRvMQ0wCwYDVQQKDARTQ0VFMQswCQYDVQQGEwJQVAIIYscgszgZna8wDQYJYIZIAWUDBAIBBQCgSDAVBgkqhkiG9w0BCQMxCAYGZ4EIAQEBMC8GCSqGSIb3DQEJBDEiBCBHmTqFNcIaJWXURbilQYBwJpVN6uR7FKrwG+UVCHPLhDANBgkqhkiG9w0BAQsFAASCAQCINsGudhOSW1QdF1xOIVjKWZN5z28ZHiYhVgwn7uuCKrN+eVmGokmLpi0iunHlnzsO2NJ5wB2d7nzqrgen+sXT2HsmXMmLaDzarKu2xabCZCuRLIYtIKmYtSbfF52LzAnNm44AV139ZwoCipSEInicRlIw3OAwue8hdrUPkhBgMhzc4AlrMU7zZ+f5h/QNf0b68qNhu3KFzCSykW46yctLGRY+M0pL2w0Flc5KWuGL8U46t4qRlkG6e9+gOwRuIqczNOXqpRgok7uQLIWr3GiFmcnYzay3f+w/+k2fMIy5rFdKe/1SUSnleluTQG/peXx2934eGt2kV9onoDtMUP1+AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==',
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#       EF07 - Perso Data
data["3f00-5f00-ef07"] = {'data': b2a_base64(bytes([0] * 1000)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#       EF09 - Cert Auth
data["3f00-5f00-ef09"] = {'data': b2a_base64(user_auth.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

data["auth-private-key"] = {'data': b2a_base64(user_auth_private_key.private_bytes(encoding=serialization.Encoding.DER,
                                    format=serialization.PrivateFormat.TraditionalOpenSSL,
                                    encryption_algorithm=serialization.NoEncryption()))}

#       EF08 - Cert Sign
data["3f00-5f00-ef08"] = {'data': b2a_base64(user_sign.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

data["sign-private-key"] = {'data': b2a_base64(user_sign_private_key.private_bytes(encoding=serialization.Encoding.DER,
                format=serialization.PrivateFormat.TraditionalOpenSSL,
                encryption_algorithm=serialization.NoEncryption()))}

#       EF11 - Cert Root
data["3f00-5f00-ef11"] = {'data': b2a_base64(ec_raizestado.public_bytes(encoding=serialization.Encoding.DER))}

#       EF10 - Cert Root Auth
data["3f00-5f00-ef10"] = {'data': b2a_base64(ec_auth.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}

#       EF0F - Cert Root Sign
data["3f00-5f00-ef0f"] = {'data': b2a_base64(ec_sign.public_bytes(encoding=serialization.Encoding.DER)),
                          'fci': b2a_base64(b'\x8C\x05\x1B\xFF\xFF\xFF\x00')}


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
