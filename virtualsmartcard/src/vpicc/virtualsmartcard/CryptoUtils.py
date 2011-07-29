#
# Copyright (C) 2009 Dominik Oepen
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
import sys, binascii, utils, random
from Crypto.Cipher import DES3, DES, AES, ARC4 #,IDEA no longer present in python-crypto?
from struct import pack
from binascii import b2a_hex
from random import randint
from virtualsmartcard.utils import inttostring, stringtoint, hexdump
import string
import re

try:
    # Use PyCrypto (if available)
    from Crypto.Hash import HMAC, SHA as SHA1

except ImportError:
    # PyCrypto not available.  Use the Python standard library.
    import hmac as HMAC
    import sha as SHA1

CYBERFLEX_IV = '\x00' * 8

## *******************************************************************
## * Generic methods                                                 *
## *******************************************************************
def get_cipher(cipherspec, key, iv = None):       
    cipherparts = cipherspec.split("-")
    
    if len(cipherparts) > 2:
        raise ValueError, 'cipherspec must be of the form "cipher-mode" or "cipher"'
    elif len(cipherparts) == 1:
        cipherparts[1] = "ecb"
    
    c_class = globals().get(cipherparts[0].upper(), None)
    if c_class is None: 
        raise ValueError, "Cipher '%s' not known, must be one of %s" % (cipherparts[0], ", ".join([e.lower() for e in dir() if e.isupper()]))
    
    mode = getattr(c_class, "MODE_" + cipherparts[1].upper(), None)
    if mode is None:
        raise ValueError, "Mode '%s' not known, must be one of %s" % (cipherparts[1], ", ".join([e.split("_")[1].lower() for e in dir(c_class) if e.startswith("MODE_")]))
    
    cipher = None
    if iv is None:
        cipher = c_class.new(key, mode)
    else:
        cipher = c_class.new(key, mode, iv)

    return cipher

def get_cipher_keylen(cipherspec):
    cipherparts = cipherspec.split("-")
    
    if len(cipherparts) > 2:
        raise ValueError, 'cipherspec must be of the form "cipher-mode" or "cipher"'

    cipher = cipherparts[0].upper()
    #cipher = globals().get(cipherparts[0].upper(), None)
    #Note: return cipher.key_size does not work on Ubuntu, because e.g. AES.key_size == 0
    if cipher == "AES": #Pycrypto uses AES128 
        return 16
    elif cipher == "DES":
        return 8
    elif cipher == "DES3":
        return 16
    else:
       raise ValueError, "Unsupported Encryption Algorithm: " + cipher

def get_cipher_blocklen(cipherspec):
    cipherparts = cipherspec.split("-")
    
    if len(cipherparts) > 2:
        raise ValueError, 'cipherspec must be of the form "cipher-mode" or "cipher"'

    cipher = globals().get(cipherparts[0].upper(), None)
    return cipher.block_size

def append_padding(cipherspec, data, padding_class=0x01):
    """Append padding to the data. 
    Length of padding depends on length of data and the block size of the
    specified encryption algorithm.
    Different types of padding may be selected via the padding_class parameter
    """
    blocklen = get_cipher_blocklen(cipherspec)

    if padding_class == 0x01: #ISO padding
        last_block_length = len(data) % blocklen
        padding_length = blocklen - last_block_length
        if padding_length == 0:
            padding = '\x80' + '\x00' * (blocklen - 1)
        else:
            padding = '\x80' + '\x00' * (blocklen - last_block_length - 1)

    return data + padding

def strip_padding(cipherspec,data,padding_class=0x01):
    """
    Strip the padding of decrypted data. Returns data without padding
    """
    
    #TODO: Sanity check
    if padding_class == 0x01:
        tail = len(data) - 1
        while data[tail] != '\x80':
            tail = tail - 1
        return data[:tail]
    
def crypto_checksum(algo, key, data, iv=None, ssc=None):
    if algo not in ("HMAC","MAC","CC"):
        raise ValueError, "Unknown Algorithm %s" % algo
    
    if algo == "MAC":
        checksum = calculate_MAC(key,data,0x00,iv) #FIXME: IV?
    elif algo == "HMAC":
        hmac = HMAC.new(key,data)
        checksum = hmac.hexdigest()
        del hmac
    elif algo == "CC":
        if ssc != None:
            data = inttostring(ssc) + data        
        a = cipher(True, "des-cbc", key[:8], data)
        b = cipher(False, "des-ecb", key[8:16], a[-8:])
        c = cipher(True, "des-ecb", key[:8], b)
        checksum = c
    
    return checksum

def cipher(do_encrypt, cipherspec, key, data, iv = None):
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

def encrypt(cipherspec, key, data, iv = None):
    return cipher(True, cipherspec, key, data, iv)

def decrypt(cipherspec, key, data, iv = None):
    return cipher(False, cipherspec, key, data, iv)

def hash(hashmethod,data):
    from Crypto.Hash import SHA, MD5#, RIPEMD
    hash_class = locals().get(hashmethod.upper(), None)
    if hash_class == None:
        print "Unknown Hash method %s" % hashmethod
        raise ValueError
    hash = hash_class.new()
    hash.update(data)
    return hash.digest()
   
def operation_on_string(string1, string2, op):
    if len(string1) != len(string2):
        raise ValueError, "string1 and string2 must be of equal length"
    result = []
    for i in range(len(string1)):
        result.append( chr(op(ord(string1[i]),ord(string2[i]))) )
    return "".join(result)

def protect_string(string, password, cipherspec=None):
    """
    Encrypt and authenticate a string
    """
     
    #Derive a key and a salt from password
    pbk = crypt(password)
    regex = re.compile('\$p5k2\$\$[\w./]*\$')
    match = regex.match(pbk)
    if match != None:
        salt = pbk[7:match.end()-1]
        key = pbk[match.end():]
    else:
        raise ValueError
    
    #Encrypt the string, authenticate and format it
    if cipherspec == None:
        cipherspec = "AES-CBC"
    else:
        pass #TODO: Sanity check for cipher
    paddedString = append_padding(cipherspec, string)                
    cryptedString = cipher(True, cipherspec, key, paddedString)
    hmac = crypto_checksum("HMAC", key, cryptedString)
    protectedString = "$p5k2$$" + salt + "$" + cryptedString + hmac
    
    return protectedString

def read_protected_string(string, password, cipherspec=None):
    """
    Decrypt a protected string and verify the authentication data
    """
    hmac_length = 32 #FIXME: Ugly

    #Check if the string has the structure, that is generated by protect_string 
    
    regex = re.compile('\$p5k2\$\$[\w./]*\$') #TODO: Ensure the right format!
    match = regex.match(string)
    if match != None:
        crypted = string[match.end():len(string) - hmac_length]
        salt = string[7:match.end() - 1]
        hmac = string[len(string) - hmac_length:]
    else:
        raise ValueError, "Wrong string format"
    
    #Derive key
    pbk = crypt(password, salt)
    match = regex.match(pbk)
    key = pbk[match.end():]
    
    #Verify HMAC
    checksum = crypto_checksum("HMAC", key, crypted)
    if checksum != hmac:
        print "Found HMAC %s expected %s" % (str(hmac), str(checksum))
        raise ValueError, "Failed to authenticate data. Wrong password?"
    
    #Decrypt data
    if cipherspec == None:
        cipherspec = "AES-CBC"
    decrypted = cipher(False, cipherspec, key, crypted)
        
    return strip_padding(cipherspec, decrypted)  

## *******************************************************************
## * Cyberflex specific methods                                      *
## *******************************************************************
def verify_card_cryptogram(session_key, host_challenge, card_challenge, card_cryptogram):
    message = host_challenge + card_challenge
    expected = calculate_MAC(session_key, message, CYBERFLEX_IV)
        
    return card_cryptogram == expected

def calculate_host_cryptogram(session_key, card_challenge, host_challenge):
    message = card_challenge + host_challenge
    return calculate_MAC(session_key, message, CYBERFLEX_IV)

def calculate_MAC(session_key, message, iv=CYBERFLEX_IV):
    """"
    Cyberflex MAC is the last Block of the input encrypted with DES3-CBC
    """
    
    cipher = DES3.new(session_key, DES3.MODE_CBC, iv)
    padded = append_padding("DES3", message, 0x01)
    block_count = len(padded) / cipher.block_size
    crypted = cipher.encrypt(padded)
   
    return crypted[len(padded) - cipher.block_size : ]

def get_derivation_data(host_challenge, card_challenge):
    return card_challenge[4:8] + host_challenge[:4] + \
        card_challenge[:4] + host_challenge[4:8]

def get_session_key(auth_key, host_challenge, card_challenge):
    cipher = DES3.new(auth_key, DES3.MODE_ECB)
    return cipher.encrypt(get_derivation_data(host_challenge, card_challenge))

def generate_host_challenge():
    random.seed()
    return "".join([chr(random.randint(0,255)) for e in range(8)])

def andstring(string1, string2):
    return operation_on_string(string1, string2, lambda a,b: a & b)

###########################################################################
# PBKDF2.py - PKCS#5 v2.0 Password-Based Key Derivation
#
# Copyright (C) 2007, 2008 Dwayne C. Litzenberger <dlitz@dlitz.net>
# All rights reserved.
# 
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies and that
# both that copyright notice and this permission notice appear in
# supporting documentation.
# 
# THE AUTHOR PROVIDES THIS SOFTWARE ``AS IS'' AND ANY EXPRESSED OR 
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Country of origin: Canada
#
###########################################################################
# Sample PBKDF2 usage:
#   from Crypto.Cipher import AES
#   from PBKDF2 import PBKDF2
#   import os
#
#   salt = os.urandom(8)    # 64-bit salt
#   key = PBKDF2("This passphrase is a secret.", salt).read(32) # 256-bit key
#   iv = os.urandom(16)     # 128-bit IV
#   cipher = AES.new(key, AES.MODE_CBC, iv)
#     ...
#
# Sample crypt() usage:
#   from PBKDF2 import crypt
#   pwhash = crypt("secret")
#   alleged_pw = raw_input("Enter password: ")
#   if pwhash == crypt(alleged_pw, pwhash):
#       print "Password good"
#   else:
#       print "Invalid password"
#
###########################################################################
# History:
#
#  2007-07-27 Dwayne C. Litzenberger <dlitz@dlitz.net>
#   - Initial Release (v1.0)
#
#  2007-07-31 Dwayne C. Litzenberger <dlitz@dlitz.net>
#   - Bugfix release (v1.1)
#   - SECURITY: The PyCrypto XOR cipher (used, if available, in the _strxor
#   function in the previous release) silently truncates all keys to 64
#   bytes.  The way it was used in the previous release, this would only be
#   problem if the pseudorandom function that returned values larger than
#   64 bytes (so SHA1, SHA256 and SHA512 are fine), but I don't like
#   anything that silently reduces the security margin from what is
#   expected.
#  
# 2008-06-17 Dwayne C. Litzenberger <dlitz@dlitz.net>
#   - Compatibility release (v1.2)
#   - Add support for older versions of Python (2.2 and 2.3).
#
###########################################################################

__version__ = "1.2"

def strxor(a, b):
    return "".join([chr(ord(x) ^ ord(y)) for (x, y) in zip(a, b)])

def b64encode(data, chars="+/"):
    tt = string.maketrans("+/", chars)
    return data.encode('base64').replace("\n", "").translate(tt)

class PBKDF2(object):
    """PBKDF2.py : PKCS#5 v2.0 Password-Based Key Derivation
    
    This implementation takes a passphrase and a salt (and optionally an
    iteration count, a digest module, and a MAC module) and provides a
    file-like object from which an arbitrarily-sized key can be read.

    If the passphrase and/or salt are unicode objects, they are encoded as
    UTF-8 before they are processed.

    The idea behind PBKDF2 is to derive a cryptographic key from a
    passphrase and a salt.
    
    PBKDF2 may also be used as a strong salted password hash.  The
    'crypt' function is provided for that purpose.
    
    Remember: Keys generated using PBKDF2 are only as strong as the
    passphrases they are derived from.
    """

    def __init__(self, passphrase, salt, iterations=1000,
                 digestmodule=SHA1, macmodule=HMAC):
        self.__macmodule = macmodule
        self.__digestmodule = digestmodule
        self._setup(passphrase, salt, iterations, self._pseudorandom)

    def _pseudorandom(self, key, msg):
        """Pseudorandom function.  e.g. HMAC-SHA1"""
        return self.__macmodule.new(key=key, msg=msg,
            digestmod=self.__digestmodule).digest()
    
    def read(self, bytes):
        """Read the specified number of key bytes."""
        if self.closed:
            raise ValueError("file-like object is closed")

        size = len(self.__buf)
        blocks = [self.__buf]
        i = self.__blockNum
        while size < bytes:
            i += 1
            if i > 0xffffffffL or i < 1:
                # We could return "" here, but 
                raise OverflowError("derived key too long")
            block = self.__f(i)
            blocks.append(block)
            size += len(block)
        buf = "".join(blocks)
        retval = buf[:bytes]
        self.__buf = buf[bytes:]
        self.__blockNum = i
        return retval
    
    def __f(self, i):
        # i must fit within 32 bits
        assert 1 <= i <= 0xffffffffL
        U = self.__prf(self.__passphrase, self.__salt + pack("!L", i))
        result = U
        for j in xrange(2, 1+self.__iterations):
            U = self.__prf(self.__passphrase, U)
            result = strxor(result, U)
        return result
    
    def hexread(self, octets):
        """Read the specified number of octets. Return them as hexadecimal.

        Note that len(obj.hexread(n)) == 2*n.
        """
        return b2a_hex(self.read(octets))

    def _setup(self, passphrase, salt, iterations, prf):
        # Sanity checks:
        
        # passphrase and salt must be str or unicode (in the latter
        # case, we convert to UTF-8)
        if isinstance(passphrase, unicode):
            passphrase = passphrase.encode("UTF-8")
        if not isinstance(passphrase, str):
            raise TypeError("passphrase must be str or unicode")
        if isinstance(salt, unicode):
            salt = salt.encode("UTF-8")
        if not isinstance(salt, str):
            raise TypeError("salt must be str or unicode")

        # iterations must be an integer >= 1
        if not isinstance(iterations, (int, long)):
            raise TypeError("iterations must be an integer")
        if iterations < 1:
            raise ValueError("iterations must be at least 1")
        
        # prf must be callable
        if not callable(prf):
            raise TypeError("prf must be callable")

        self.__passphrase = passphrase
        self.__salt = salt
        self.__iterations = iterations
        self.__prf = prf
        self.__blockNum = 0
        self.__buf = ""
        self.closed = False
    
    def close(self):
        """Close the stream."""
        if not self.closed:
            del self.__passphrase
            del self.__salt
            del self.__iterations
            del self.__prf
            del self.__blockNum
            del self.__buf
            self.closed = True

def crypt(word, salt=None, iterations=None):
    """PBKDF2-based unix crypt(3) replacement.
    
    The number of iterations specified in the salt overrides the 'iterations'
    parameter.

    The effective hash length is 192 bits.
    """
    
    # Generate a (pseudo-)random salt if the user hasn't provided one.
    if salt is None:
        salt = _makesalt()

    # salt must be a string or the us-ascii subset of unicode
    if isinstance(salt, unicode):
        salt = salt.encode("us-ascii")
    if not isinstance(salt, str):
        raise TypeError("salt must be a string")

    # word must be a string or unicode (in the latter case, we convert to UTF-8)
    if isinstance(word, unicode):
        word = word.encode("UTF-8")
    if not isinstance(word, str):
        raise TypeError("word must be a string or unicode")

    # Try to extract the real salt and iteration count from the salt
    if salt.startswith("$p5k2$"):
        (iterations, salt, dummy) = salt.split("$")[2:5]
        if iterations == "":
            iterations = 400
        else:
            converted = int(iterations, 16)
            if iterations != "%x" % converted:  # lowercase hex, minimum digits
                raise ValueError("Invalid salt")
            iterations = converted
            if not (iterations >= 1):
                raise ValueError("Invalid salt")
    
    # Make sure the salt matches the allowed character set
    allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./"
    for ch in salt:
        if ch not in allowed:
            raise ValueError("Illegal character %r in salt" % (ch,))

    if iterations is None or iterations == 400:
        iterations = 400
        salt = "$p5k2$$" + salt
    else:
        salt = "$p5k2$%x$%s" % (iterations, salt)
    rawhash = PBKDF2(word, salt, iterations).read(24)
    # return salt + "$" + b64encode(rawhash, "./") DO: Original return line
    return salt + "$" + rawhash
# Add crypt as a static method of the PBKDF2 class
# This makes it easier to do "from PBKDF2 import PBKDF2" and still use
# crypt.
PBKDF2.crypt = staticmethod(crypt)

def _makesalt():
    """Return a 48-bit pseudorandom salt for crypt().
    
    This function is not suitable for generating cryptographic secrets.
    """
    binarysalt = "".join([pack("@H", randint(0, 0xffff)) for i in range(3)])
    return b64encode(binarysalt, "./")

def test_pbkdf2():
    """Module self-test"""
    from binascii import a2b_hex
    
    #
    # Test vectors from RFC 3962
    #

    # Test 1
    result = PBKDF2("password", "ATHENA.MIT.EDUraeburn", 1).read(16)
    expected = a2b_hex("cdedb5281bb2f801565a1122b2563515")
    if result != expected:
        raise RuntimeError("self-test failed")

    # Test 2
    result = PBKDF2("password", "ATHENA.MIT.EDUraeburn", 1200).hexread(32)
    expected = ("5c08eb61fdf71e4e4ec3cf6ba1f5512b"
                "a7e52ddbc5e5142f708a31e2e62b1e13")
    if result != expected:
        raise RuntimeError("self-test failed")

    # Test 3
    result = PBKDF2("X"*64, "pass phrase equals block size", 1200).hexread(32)
    expected = ("139c30c0966bc32ba55fdbf212530ac9"
                "c5ec59f1a452f5cc9ad940fea0598ed1")
    if result != expected:
        raise RuntimeError("self-test failed")
    
    # Test 4
    result = PBKDF2("X"*65, "pass phrase exceeds block size", 1200).hexread(32)
    expected = ("9ccad6d468770cd51b10e6a68721be61"
                "1a8b4d282601db3b36be9246915ec82a")
    if result != expected:
        raise RuntimeError("self-test failed")
    
    #
    # Other test vectors
    #
    
    # Chunked read
    f = PBKDF2("kickstart", "workbench", 256)
    result = f.read(17)
    result += f.read(17)
    result += f.read(1)
    result += f.read(2)
    result += f.read(3)
    expected = PBKDF2("kickstart", "workbench", 256).read(40)
    if result != expected:
        raise RuntimeError("self-test failed")
    
    #
    # crypt() test vectors
    #

    # crypt 1
    result = crypt("cloadm", "exec")
    expected = '$p5k2$$exec$r1EWMCMk7Rlv3L/RNcFXviDefYa0hlql'
    if result != expected:
        raise RuntimeError("self-test failed")
    
    # crypt 2
    result = crypt("gnu", '$p5k2$c$u9HvcT4d$.....')
    expected = '$p5k2$c$u9HvcT4d$Sd1gwSVCLZYAuqZ25piRnbBEoAesaa/g'
    if result != expected:
        raise RuntimeError("self-test failed")

    # crypt 3
    result = crypt("dcl", "tUsch7fU", iterations=13)
    expected = "$p5k2$d$tUsch7fU$nqDkaxMDOFBeJsTSfABsyn.PYUXilHwL"
    if result != expected:
        raise RuntimeError("self-test failed")
    
    # crypt 4 (unicode)
    result = crypt(u'\u0399\u03c9\u03b1\u03bd\u03bd\u03b7\u03c2',
        '$p5k2$$KosHgqNo$9mjN8gqjt02hDoP0c2J0ABtLIwtot8cQ')
    expected = '$p5k2$$KosHgqNo$9mjN8gqjt02hDoP0c2J0ABtLIwtot8cQ'
    if result != expected:
        raise RuntimeError("self-test failed")
    print "PBKDF2 self test successfull"
    
if __name__ == "__main__":
    default_key = binascii.a2b_hex("404142434445464748494A4B4C4D4E4F")
    
    host_chal = binascii.a2b_hex("".join("89 45 19 BF BC 1A 5B D8".split()))
    card_chal = binascii.a2b_hex("".join("27 4D B7 EA CA 66 CE 44".split()))
    card_crypto = binascii.a2b_hex("".join("8A D4 A9 2D 9B 6B 24 E0".split()))
    
    session_key = get_session_key(default_key, host_chal, card_chal)
    print "Session-Key:  ", utils.hexdump(session_key)
    
    print verify_card_cryptogram(session_key, host_chal, card_chal, card_crypto)
    
    host_crypto = calculate_host_cryptogram(session_key, card_chal, host_chal)
    print "Host-Crypto:  ", utils.hexdump( host_crypto )

    external_authenticate = binascii.a2b_hex("".join("84 82 01 00 10".split())) + host_crypto
    print utils.hexdump(calculate_MAC(session_key, external_authenticate))
    
    too_short = binascii.a2b_hex("".join("89 45 19 BF".split()))
    padded = append_padding("DES3-ECB", too_short)
    print "Padded data: " + utils.hexdump(padded)
    unpadded = strip_padding("DES3-ECB", padded)
    print "Without padding: " + utils.hexdump(unpadded)
    
    teststring = "DEADBEEFistatsyksdvhihewohfwoehcowc8hw8rogfq8whv75tsgohsav8wress"
    foo = append_padding("AES", teststring)
    print len(foo)
    print strip_padding("AES", foo)
    testpass = "SomeRandomPassphrase"
    protectedString = protect_string(teststring, testpass)
    unprotectedString = read_protected_string(protectedString, testpass)
    if teststring != unprotectedString:
        print "protect_string test failed"
    
    #test_pbkdf2()
