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
import re
from struct import pack
from base64 import b64encode
from random import randint
from virtualsmartcard.utils import inttostring

try:
    # Use PyCrypto (if available)
    from Crypto.Cipher import DES3, DES, AES, ARC4  # @UnusedImport
    from Crypto.Hash import HMAC, SHA as SHA1

except ImportError:
    # PyCrypto not available.  Use the Python standard library.
    import hmac as HMAC
    import sha as SHA1

CYBERFLEX_IV = b'\x00' * 8


# *******************************************************************
# * Generic methods                                                 *
# *******************************************************************
def get_cipher(cipherspec, key, iv=None):
    cipherparts = cipherspec.split("-")

    if len(cipherparts) > 2:
        raise ValueError('cipherspec must be of the form "cipher-mode" or'
                         '"cipher"')
    elif len(cipherparts) == 1:
        cipherparts[1] = "ecb"

    c_class = globals().get(cipherparts[0].upper(), None)
    if c_class is None:
        validCiphers = ",".join([e.lower() for e in dir() if e.isupper()])
        raise ValueError("Cipher '%s' not known, must be one of %s" %
                         (cipherparts[0], validCiphers))

    mode = getattr(c_class, "MODE_" + cipherparts[1].upper(), None)
    if mode is None:
        validModes = ", ".join([e.split("_")[1].lower() for e in dir(c_class)
                               if e.startswith("MODE_")])
        raise ValueError("Mode '%s' not known, must be one of %s" %
                         (cipherparts[1], validModes))

    cipher = None
    if cipherparts[1].upper() == "ECB":
        cipher = c_class.new(key, mode)
    elif iv is None:
        cipher = c_class.new(key, mode, b'\x00'*get_cipher_blocklen(cipherspec))
    else:
        cipher = c_class.new(key, mode, iv)

    return cipher


def get_cipher_keylen(cipherspec):
    cipherparts = cipherspec.split("-")

    if len(cipherparts) > 2:
        raise ValueError('cipherspec must be of the form "cipher-mode" or'
                         '"cipher"')

    cipher = cipherparts[0].upper()
    # cipher = globals().get(cipherparts[0].upper(), None)
    # Note: return cipher.key_size does not work on Ubuntu, because e.g.
    # AES.key_size == 0
    if cipher == "AES":  # Pycrypto uses AES128
        return 16
    elif cipher == "DES":
        return 8
    elif cipher == "DES3":
        return 16
    else:
        raise ValueError("Unsupported Encryption Algorithm: " + cipher)


def get_cipher_blocklen(cipherspec):
    cipherparts = cipherspec.split("-")

    if len(cipherparts) > 2:
        raise ValueError('cipherspec must be of the form "cipher-mode" or'
                         '"cipher"')

    cipher = globals().get(cipherparts[0].upper(), None)
    return cipher.block_size


def append_padding(blocklen, data, padding_class=0x01):
    """Append padding to the data.
    Length of padding depends on length of data and the block size of the
    specified encryption algorithm.
    Different types of padding may be selected via the padding_class parameter
    """

    if padding_class == 0x01:  # ISO padding
        last_block_length = len(data) % blocklen
        padding_length = blocklen - last_block_length
        if padding_length == 0:
            padding = b'\x80' + b'\x00' * (blocklen - 1)
        else:
            padding = b'\x80' + b'\x00' * (blocklen - last_block_length - 1)

    return data + padding


def strip_padding(blocklen, data, padding_class=0x01):
    """
    Strip the padding of decrypted data. Returns data without padding
    """

    if padding_class == 0x01:
        b = data
        if isinstance(b, str):
            b = map(ord, b)
        tail = len(b) - 1
        while b[tail] != 0x80:
            tail = tail - 1
        return data[:tail]


def crypto_checksum(algo, key, data, iv=None, ssc=None):
    """
    Compute various types of cryptographic checksums.

    :param algo:
        A string specifying the algorithm to use. Currently supported
        algorithms are \"MAX\" \"HMAC\" and \"CC\" (Meaning a cryptographic
        checksum as used by the ICAO passports)
    :param key:  They key used to computed the cryptographic checksum
    :param data: The data for which to calculate the checksum
    :param iv:
        Optional. An initialization vector. Only used by the \"MAC\" algorithm
    :param ssc:
        Optional. A send sequence counter to be prepended to the data.
        Only used by the \"CC\" algorithm

    """

    if algo not in ("HMAC", "MAC", "CC"):
        raise ValueError("Unknown Algorithm %s" % algo)

    if algo == "MAC":
        checksum = calculate_MAC(key, data, iv)
    elif algo == "HMAC":
        hmac = HMAC.new(key, data)
        checksum = hmac.hexdigest()
        del hmac
    elif algo == "CC":
        if ssc is not None:
            data = inttostring(ssc) + data
        a = cipher(True, "des-cbc", key[:8], data)
        b = cipher(False, "des-ecb", key[8:16], a[-8:])
        c = cipher(True, "des-ecb", key[:8], b)
        checksum = c

    return checksum


def cipher(do_encrypt, cipherspec, key, data, iv=None):
    """Do a cryptographic operation.
    operation = do_encrypt ? encrypt : decrypt,
    cipherspec must be of the form "cipher-mode", or "cipher\""""

    cipher = get_cipher(cipherspec, key, iv)

    result = None
    if do_encrypt:
        result = cipher.encrypt(data)
    else:
        result = cipher.decrypt(data)

    del cipher
    return result


def encrypt(cipherspec, key, data, iv=None):
    return cipher(True, cipherspec, key, data, iv)


def decrypt(cipherspec, key, data, iv=None):
    return cipher(False, cipherspec, key, data, iv)


def hash(hashmethod, data):
    from Crypto.Hash import SHA, MD5  # , RIPEMD
    hash_class = locals().get(hashmethod.upper(), None)
    if hash_class is None:
        logging.error("Unknown Hash method %s" % hashmethod)
        raise ValueError
    hash = hash_class.new()
    hash.update(data)
    return hash.digest()


def operation_on_string(string1, string2, op):
    if len(string1) != len(string2):
        raise ValueError("string1 and string2 must be of equal length")
    result = []
    for i in range(len(string1)):
        result.append(chr(op(ord(string1[i]), ord(string2[i]))))
    return "".join(result)


# *******************************************************************
# * Cyberflex specific methods                                      *
# *******************************************************************
def calculate_MAC(session_key, message, iv=CYBERFLEX_IV):
    """
    Cyberflex MAC is the last Block of the input encrypted with DES3-CBC
    """

    cipher = DES3.new(session_key, DES3.MODE_CBC, iv)
    padded = append_padding(8, message, 0x01)
    crypted = cipher.encrypt(padded)

    return crypted[len(padded) - cipher.block_size:]

def _makesalt():
    """Return a 48-bit pseudorandom salt for crypt().

    This function is not suitable for generating cryptographic secrets.
    """
    binarysalt = "".join([pack("@H", randint(0, 0xffff)) for i in range(3)])
    return b64encode(binarysalt, "./")
