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
import SmartcardFilesystem, CryptoUtils, TLVutils
import random, re, struct, Crypto
import hashlib
from SWutils import SwError, SW, SW_MESSAGES
from pickle import dumps, loads
from utils import inttostring, stringtoint, hexdump, C_APDU, R_APDU
from Crypto.Cipher import DES3
from ConstantDefinitions import *
from SEutils import ControlReferenceTemplate as CRT

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

    if (ciphertable.has_key(p1)):
        return ciphertable[p1]
    else:
        raise SwError(SW["ERR_INCORRECTP1P2"])

class CardContainer:
    """
    This class is used to store the data needed by the SAM
    It includes the PIN, the master key of the SAM and a
    hashmap containing all the keys used by the file encryption
    system. The keys in the hashmap are indexed via the path
    to the corresponding container.
    """
    
    def __init__(self, PIN=None, cardNumber=None, cardSecret=None):
        from os import urandom
        
        self.cardNumber = cardNumber
        self.PIN = PIN
        self.FSkeys = {}
        self.cipher = 0x01 # Algorithm reference defined in __get_referenced_algorithm(p1)
        self.master_password = None
        self.master_key = None
        self.salt = None
        self.asym_key = None
        
        keylen = CryptoUtils.get_cipher_keylen(get_referenced_cipher(self.cipher))
        if cardSecret is None: #Generate a random card secret
            self.cardSecret = urandom(keylen)
        else:
            if len(cardSecret) != keylen:
                raise ValueError, "cardSecret has the wrong key length for: " + get_referenced_cipher(self.cipher)
            else:
                self.cardSecret = cardSecret            

    def __delete__(self):
        print "Smartcard configuration is NOT saved!!!"
    
    def getPIN(self):
        return self.PIN
    
    def setPIN(self,PIN):
        self.PIN = PIN
    
    def getCardSecret(self):
        return self.cardSecret
    
    def addKey(self,path,key):
        """
        Encrypt key and store it in the SAM
        """
        crypted_key = CryptoUtils.cipher(True,self.cipher,self.master_password,key)
        if self.FSkeys.has_key(path):
            print "Overwriting key for path %s" % path
        self.FSkeys[path] = crypted_key
    
    def removeKey(self,path):
        """
        Erase a key from the SAM
        """
        if(self.FSkeys.has_key(path)):
            del self.FSkeys[path]
        else:
            raise ValueError 
    
    def getKey(self,path):
        """
        Fetch a key from the SAM, decrypt and return it
		@return: key if the key is in the container, otherwise None
        """
        if(self.FSkeys.has_key(path)):
            plain_key = CryptoUtils.cipher(False,self.cipher, self.master_password,self.FSkeys[path]) #TODO: Error checking:
            return plain_key
        else:
            return None
                
class SAM(object):
       
    def __init__(self, PIN, cardNumber, mf=None):

        self.CardContainer = CardContainer(PIN, cardNumber)      
        #self.CardContainer.loadConfiguration(path, password)
        self.mf = mf

        self.cardNumber = cardNumber
        self.cardSecret = self.CardContainer.cardSecret

        self.last_challenge = None #Will contain non-readable binary string
        self.counter = 3 #Number of tries for PIN validation

        self.SM_handler = Secure_Messaging(self.mf)

    def set_MF(self,mf):
        self.mf = mf
        self.SM_handler.set_MF(mf)

    def addFSkey(self,path,key):
        #Deprecated ?
        self.CardContainer.addKey(path, key)
    
    def delFSkey(self,path):
        #Deprecated ?
        self.CardContainer.removeKey(path)
        
    def FSencrypt(self,data):
        """
        Encrypt the given data, using the parameters stored in the SAM
        """
        if self.CardContainer.master_key == None:
            raise ValueError
        
        cipher = get_referenced_cipher(self.CardContainer.cipher)
        padded_data = CryptoUtils.append_padding(cipher,data)
        crypted_data = CryptoUtils.cipher(True,cipher,self.CardContainer.master_key,padded_data)
        return crypted_data

    def FSdecrypt(self,data):
        """
        Decrypt the given data, using the parameters stored in the SAM
        """
        if self.CardContainer.master_key == None:
            raise ValueError, "No master key set."

        cipher = get_referenced_cipher(self.CardContainer.cipher)                
        decrypted_data = CryptoUtils.cipher(False,cipher,self.CardContainer.master_key,data)
        unpadded_data = CryptoUtils.strip_padding(cipher,decrypted_data)
        return unpadded_data
    
    def set_algorithm(self,algo):
        if not algo in range(0x01,0x06):
            raise ValueError, "Illegal Parameter"
        else:
            self.CardContainer.cipher = algo

    def set_asym_algorithm(self,cipher,type):
        """
        @param cipher: Public/private key object from Crypto.PublicKey used for encryption   
        @param type: Type of the public key (e.g. RSA, DSA) 
        """
        if not type in range(0x07,0x08):
            raise ValueError, "Illegal Parameter"
        else:
            self.CardContainer.cipher = type
            self.CardContainer.asym_key = cipher
   
    def verify(self,p1,p2,PIN):        
        """
        Authenticate the card user. Check if he entered a valid PIN.
        If the PIN is invalid decrement retry counter. If retry counter 
        equals zero, block the card until reset with correct PUK
        """
        
        print "Received PIN: %s" % PIN.strip()
        PIN = PIN.replace("\0","") #Strip NULL charakters
        
        if p1 != 0x00:
            raise SwError(SW["ERR_INCORRECTP1P2"])
        
        if self.counter > 0:
            if self.CardContainer.getPIN() == PIN:
                self.counter = 3
                return SW["NORMAL"], ""
            else:
                self.counter -= 1
                print self.CardContainer.getPIN() + " != " + PIN
                raise SwError(SW["WARN_NOINFO63"])
                #raise SwError(0X63C0 + self.counter) #Be verbose
        else:
            raise SwError(SW["ERR_AUTHBLOCKED"])

    def change_reference_data(self,p1,p2,data):
        """
        Change the specified referenced data (e.g. CHV) of the card
        """
        
        data = data.replace("\0","") #Strip NULL charakters
        self.CardContainer.setPIN(data)
        print "New PIN = %s" % self.CardContainer.getPIN()
        return SW["NORMAL"], ""    

    def internal_authenticate(self,p1,p2,data):
        """
        Authenticate card to terminal. Encrypt the challenge of the terminal
        to prove key posession
        """
        
        if p1 == 0x00: #No information given
            cipher = get_referenced_cipher(self.CardContainer.cipher)   
        else:
            cipher = get_referenced_cipher(p1)

        if cipher == "RSA" or cipher == "DSA":
            crypted_challenge = self.CardContainer.asym_key.sign(data,"")
            crypted_challenge = crypted_challenge[0]
            crypted_challenge = inttostring(crypted_challenge)
        else:
            key = self._get_referenced_key(p1,p2)
            crypted_challenge = CryptoUtils.cipher(True,cipher,key,data)
        
        return SW["NORMAL"], crypted_challenge
    
    def external_authenticate(self,p1,p2,data):
        """
        Authenticate the terminal to the card. Check wether Terminal correctly
        encrypted the given challenge or not
        """
        if self.last_challenge is None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])
        
        key = self._get_referenced_key(p1,p2) 
        if p1 == 0x00: #No information given
            cipher = get_referenced_cipher(self.CardContainer.cipher)   
        else:
            cipher = get_referenced_cipher(p1)     
        
        reference = CryptoUtils.append_padding(cipher,self.last_challenge)
        reference = CryptoUtils.cipher(True,cipher,key,reference)
        if(reference == data):
            #Invalidate last challenge
            self.last_challenge = None
            return SW["NORMAL"], ""
        else:
            plain = CryptoUtils.cipher(False,cipher,key,data)
            print plain
            print "Reference: " + hexdump(reference)
            print "Data: " + hexdump(data)
            raise SwError(SW["WARN_NOINFO63"])
            #TODO: Counter for external authenticate?

    def mutual_authenticate(self,p1,p2,mutual_challenge):   
        """
        Takes an encrypted challenge in the form 
        'Terminal Challenge | Card Challenge | Card number'
        and checks it for validity. If the challenge is succesful
        the card encrypts 'Card Challenge | Terminal challenge' and
        returns this value
        """
        
        key = self._get_referenced_key(p1,p2)
        card_number = self.get_card_number()

        if (key == None):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        if p1 == 0x00: #No information given
            cipher = get_referenced_cipher(self.CardContainer.cipher)   
        else:
            cipher = get_referenced_cipher(p1)
        
        if (cipher == None):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        plain = CryptoUtils.cipher(False,cipher,key,mutual_challenge)
        terminal_challenge = plain[:len(self.last_challenge)-1]
        card_challenge = plain[len(self.last_challenge):len(challenge)-len(card_number)-1]
        serial_number = plain[(challenge)-len(card_number):]
        if terminal_challenge != self.last_challenge:
            print "Mutual authentication failed!"
            raise SwError(SW["WARN_NOINFO63"])
        elif serial_number != card_number:
            print "Mutual authentication failed!" 
            raise SwError(SW["WARN_NOINFO63"])
        
        result = card_challenge + terminal_challenge
        return SW["NORMAL"], CryptoUtils.cipher(True,cipher,key,result)
    
    def get_challenge(self,p1,p2,data):
        """
        Generate a random number of maximum 8 Byte and return it.
        """
        if (p1 != 0x00 or p2 != 0x00): #RFU
            raise SwError(SW["ERR_INCORRECTP1P2"])
        
        length = 8 #Length of the challenge in Byte
        max = "0x" + length * "ff"
        
        #cipher = get_referenced_cipher(self.CardContainer.cipher)        
        #block_size = Crypto.Cipher.cipher.block_size

        random.seed()
        self.last_challenge = random.randint(0, int(max,16))
        print "Generated challenge: %s" % str(self.last_challenge)
        self.last_challenge = inttostring(self.last_challenge,length)

        return SW["NORMAL"], self.last_challenge
    
    def get_card_number(self):
        return SW["NORMAL"], inttostring(self.cardNumber)
      
    def _get_referenced_key(self,p1,p2):
        """
        This method returns the key specified by the p2 parameter. The key may be
        stored on the cards filesystem or in memory.
		@param p1: Specifies the algorithm to use. Needed to know the keylength.
        @param p2: Specifies a reference to the key to be used for encryption
        """
		
        """ Meaning of p2:
        b8 b7 b6 b5 b4 b3 b2 b1  |  Meaning
        0  0  0  0  0  0  0  0   |  No information is given
        0  -- -- -- -- -- -- --  |  Global reference data (e.g. an MF secific key)
        1  -- -- -- -- -- -- --  |  Specific reference data (e.g. DF specific key)
        -- -- -- x  x  x  x  x   |  Number of the secret
        Any other value          |  RFU
        """
        
        key = None
        qualifier = p2 & 0x1F
        algo = get_referenced_cipher(p1)        
        keylength = CryptoUtils.get_cipher_keylen(algo)

        if (p2 == 0x00): #No information given, use the global card key
            key = self.CardContainer.cardSecret
        #elif ((p2 >> 7) == 0x01 or (p2 >> 7) == 0x00): #We treat global and specific reference data alike
        else:		
            try: #Interpret qualifier as an short fid and try to read the key from the FS
                if self.mf == None: raise SwError(SW["CONDITIONSNOTSATISFIED"])
                df = self.mf.currentDF()
                fid = df.select("fid", stringtoint(qualifier))
                key = fid.readbinary(keylength)
            except SwError:
                key = None

            if key == None: #Try to read the key from the Key Container        
                key = self.CardContainer.getKey(reference_data)
			              
        if key != None:
            return key
        else: 
            raise SwError(SW["DATANOTFOUND"])
               
    def _read_FS_data(self,fid,length):
        
        """
        This function creates a dummy filesystem, so that data (for example keys)
        can be read from the FS. It is here for testing purposes. Later on the OS
        interface to the FS should be used.
        """       
        mf = SmartcardFilesystem.MF(filedescriptor=FDB["NOTSHAREABLEFILE"],
            lifecycle=LCB["NOINFORMATION"], dfname=None)
        
        mf = SmartcardFilesystem.MF(filedescriptor=FDB["DF"],lifecycle=LCB["ACTIVATED"], dfname=None)
        df = mf.append(SmartcardFilesystem.DF(parent=mf,fid=4, dfname='\xd2\x76\x00\x01\x24\x01', bertlv_data=[]))
        mf.append(SmartcardFilesystem.TransparentStructureEF(parent=df, fid=0x1012,
           filedescriptor=0, data="Key from the FS."))

        df = mf.currentDF()
        file = df.select('fid', 0X1012)
        sw, result = file.readbinary(0)
        return sw, result

    #The following commands define the interface to the Secure Messaging functions
    def generate_public_key_pair(self,p1,p2,data):
        return self.SM_handler.generate_public_key_pair(p1, p2, data)

    def parse_SM_CAPDU(self,CAPDU,header_authentication):
        return self.SM_handler.parse_SM_CAPDU(CAPDU, header_authentication)
    
    def protect_result(self,sw,unprotected_result):
        return self.SM_handler.protect_response(sw, unprotected_result)

    def perform_security_operation(self,p1,p2,data):
        return self.SM_handler.perform_security_operation(p1, p2, data)
    
    def manage_security_environment(self,p1,p2,data):
        return self.SM_handler.manage_security_environment(p1, p2, data)

class PassportSAM(SAM):       
    def __init__(self, mf):
        df = mf.currentDF()
        ef_dg1 = df.select("fid", 0x0101)
        dg1 = ef_dg1.readbinary(5)
        self.mrz1 = dg1[:43]
        self.mrz2 = dg1[44:]
        self.KSeed = None 
        self.KEnc = None
        self.KMac = None
        self.KSenc = None
        self.KSmac = None
        self.__computeKeys()
        #SAM.__init__(self, path, key,None)
        SAM.__init__(self, None, None, mf)
        self.SM_handler = ePass_SM(mf, None, None, None)
        self.SM_handler.CAPDU_SE.cct.algo = "CC"
        self.SM_handler.RAPDU_SE.cct.algo = "CC"
        self.SM_handler.CAPDU_SE.ct.algo = "DES3-CBC"
        self.SM_handler.RAPDU_SE.ct.algo = "DES3-CBC"
        
    def __computeKeys(self):
        """Computes the keys depending on the machine readable 
        zone of the passport according to TR-PKI mrtds ICC read-only 
        access v1.1 annex E.1."""
        MRZ_information = self.mrz2[0:10] + self.mrz2[13:20] + self.mrz2[21:28]
        H = hashlib.sha1(MRZ_information).digest()
        self.KSeed = H[:16]
        self.KEnc = self.derive_key(self.KSeed, 1)
        self.KMac = self.derive_key(self.KSeed, 2)
        
    def derive_key(self, seed, c):
        """Derive a key according to TR-PKI mrtds ICC read-only access v1.1 annex E.1.
        c is either 1 for encryption or 2 for MAC computation.
        Returns: Ka + Kb
        Note: Does not adjust parity. Nobody uses that anyway ..."""
        D = seed + struct.pack(">i", c)
        H = hashlib.sha1(D).digest()
        Ka = H[0:8]
        Kb = H[8:16]
        return Ka + Kb
    
    def external_authenticate(self,p1,p2,resp_data):
        """Performes the basic access controll protocoll as defined in
        the ICAO MRTD standard"""
        rnd_icc = self.last_challenge
        
        #Receive Mutual Authenticate APDU from terminal
        #Decrypt data and check MAC
        #cipher = DES3.new(self.KEnc,"des3-cbc")
        Eifd = resp_data[:-8]
        Mifd = self._mac(self.KMac, Eifd)
        #Check the MAC
        if not Mifd == resp_data[-8:]:
            raise ValueError, "Passport authentication failed: Wrong MAC on incoming data during Mutual Authenticate"
        #Decrypt the data
        plain = CryptoUtils.cipher(False,"DES3-CBC",self.KEnc,resp_data[:-8])
        #Split decrypted data into the two nonces and 
        if plain[8:16] != rnd_icc:
            raise SwError(SW["WARN_NOINFO63"])
        #Extraxt keying material from IFD, generate ICC keying material
        Kifd = plain[16:]
        rnd_ifd = plain[:8]
        max = "0x" + 16 * "ff"
        random.seed()
        Kicc = inttostring(random.randint(0, int(max,16)))
        #Generate Answer
        data = plain[8:16] + plain[:8] + Kicc
        Eicc = CryptoUtils.cipher(True,"DES3-CBC",self.KEnc,data)
        Micc = self._mac(self.KMac,Eicc)
        #Derive the final keys
        KSseed = CryptoUtils.operation_on_string(Kicc, Kifd, lambda a,b: a^b)
        self.KSenc = self.derive_key(KSseed, 1)
        self.KSmac = self.derive_key(KSseed, 2)
        #self.ssc = rnd_icc[-4:] + rnd_ifd[-4:]
        #Set the current SE
        self.SM_handler.CAPDU_SE.ct.key = self.KSenc
        self.SM_handler.CAPDU_SE.cct.key = self.KSmac
        self.SM_handler.RAPDU_SE.ct.key = self.KSenc
        self.SM_handler.RAPDU_SE.cct.key = self.KSmac
        self.SM_handler.ssc = stringtoint(rnd_icc[-4:] + rnd_ifd[-4:])
        self.SM_handler.CAPDU_SE.ct.algo = "DES3-CBC"
        self.SM_handler.RAPDU_SE.ct.algo = "DES3-CBC"
        self.SM_handler.CAPDU_SE.cct.algo = "CC"
        self.SM_handler.RAPDU_SE.cct.algo = "CC"
        return SW["NORMAL"], Eicc + Micc
        
    def _mac(self, key, data, ssc = None, dopad=True):
        if ssc:
            data = ssc + data
        if dopad:
            topad = 8 - len(data) % 8
            data = data + "\x80" + ("\x00" * (topad-1))
        a = CryptoUtils.cipher(True, "des-cbc", key[:8], data)
        b = CryptoUtils.cipher(False, "des-ecb", key[8:16], a[-8:])
        c = CryptoUtils.cipher(True, "des-ecb", key[:8], b)
        return c
    
class CryptoflexSAM(SAM):
    def __init__(self, mf=None):
        SAM.__init__(self, None, None, mf)
        self.SM_handler = CryptoflexSM(mf)
        
    def generate_public_key_pair(self,p1,p2,data):
        asym_key = self.SM_handler.generate_public_key_pair(p1, p2, data)
        self.set_asym_algorithm(asym_key, 0x07)
        return SW["NORMAL"], ""
    
    def perform_security_operation(self,p1,p2,data):
        """
        In the cryptoflex card, this is the verify key command. A key is send
        to the card in plain text and compared to a key stored in the card.
        This is used for authentication
        @param data: Contains the key to be verified
        @return: SW[NORMAL] in case of success otherwise SW[WARN_NOINFO63] 
        """
        return SW["NORMAL"], ""
        #FIXME
        #key = self._get_referenced_key(p1,p2)
        #if key == data:
        #    return SW["NORMAL"], ""
        #else:
        #    return SW["WARN_NOINFO63"], ""
        
    def internal_authenticate(self,p1,p2,data):
	data = data[::-1] #Reverse Byte order
	sw, data = SAM.internal_authenticate(self, p1, p2, data)
	if data != "":
		data = data[::-1]
	return sw, data
    
class Security_Environment(object):
    
    def __init__(self):       
        self.SEID = None
        self.sm_objects = ""
        #Default configuration
        ct = "\x80\x01\x01\x82\x01\x00\x85\x01\x00"
        #Control Reference Tables
        self.at = CRT(TEMPLATE_AT)
        self.kat = CRT(TEMPLATE_KAT)
        self.ht = CRT(TEMPLATE_HT)
        self.cct = CRT(TEMPLATE_CCT)
        self.dst = CRT(TEMPLATE_DST)
        self.ct = CRT(TEMPLATE_CT)


    def mse(self,config):
    	structure = TLVutils.unpack(config)
    	for tag, length, value in structure:
    		if tag == TEMPLATE_AT:
    			self.at.parse_config(config)
    		elif tag == TEMPLATE_CRT:
    			self.crt.parse_config(config)
    		elif tag == TEMPLATE_CCT:
    			self.cct.parse_config(config)
    		elif tag == TEMPLATE_KAT:
    			self.kat.parse_config(config)
    		elif tag == TEMPLATE_CT:
    			self.ct.parse_config(config)
    		elif tag == TEMPLATE_DST:
    			self.dst.parse_config(config)
    		else:
    			raise ValueError
    
class Secure_Messaging(object):
    def __init__(self,MF,CAPDU_SE=None,RAPDU_SE=None):
        self.mf = MF
        if not CAPDU_SE:
            self.CAPDU_SE = Security_Environment()
        else:
            self.CAPDU_SE = CAPDU_SE
        if not RAPDU_SE:
            self.RAPDU_SE = Security_Environment()
        else:
            self.RAPDU_SE = RAPDU_SE
            
        self.current_SE = Security_Environment()
            
    def set_MF(self,mf):
        self.mf = mf

    def manage_security_environment(self,p1,p2,data):
        """
        This method is used to store, restore or erase Security Environments
        or to manipualte the various parameters of the current SE.
        P1 specifies the operation to perform, p2 is either the SEID for the
        referred SE or the tag of a control reference template
        """
        
        """ P1:
        b8 b7 b6 b5 b4 b3 b2 b1                                  Meaning
         -  -  -  1  -  -  -  - Secure messaging in command data field
         -  -  1  -  -  -  -  - Secure messaging in response data field
         -  1  -  -  -  -  -  - Computation, decipherment, internal authentication and key agreement
        1   -  -  -  -  -  -  - Verification, encipherment, external authentication and key agreement
         -  -  -  -  0  0  0 1  SET
        1  1  1  1  0  0  1  0  STORE
        1  1  1  1  0  0  1  1  RESTORE
        1  1  1  1  0  1  0  0  ERASE
        """
               
        cmd = p1 & 0x0F
        se = p1 & 0xF0
        if(cmd == 0x01):
            if se == 0x01: #Set the right SE
                self.current_SE = self.CAPDU_SE
            elif se == 0x02:
                self.current_SE = self.RAPDU_SE
            else:
                pass #FIXME
            self.__set_SE(p2,data)
        elif(cmd== 0x02):
            self.__store_SE(p2)
        elif(cmd == 0x03):
            self.__restore_SE(p2)
        elif(cmd == 0x04):
            self.__erase_SE(p2)
        else:
            raise SwError(SW["ERR_INCORRECTP1P2"])

    def __set_SE(self,p2,data):
        """
        Manipulate the current Security Environment. P2 is the tag of a
        control reference template, data contains control reference objects
        """
        
        valid_p2 = (0xA4,0xA6,0xB4,0xB6,0xB8)
        if not p2 in valid_p2:
            raise SwError(SW["ERR_INCORRECTP1P2"])
        tag, length, value = TLVutils.unpack(data) #Only one object?
        if p2 == 0xA4:
            self.current_SE.at.parse_SE_config(value)
        elif p2 == 0xA6:
            self.current_SE.kat.parse_SE_config(value)
        elif p2 == 0xAA:
            self.current_SE.ht.parse_SE_config(value)
        elif p2 == 0xB4:
            self.current_SE.cct.parse_SE_config(value)
        elif p2 == 0xB6:
            self.current_SE.dst.parse_SE_config(value)
        elif p2 == 0xB8:
            self.current_SE.ct.parse_SE_config(value)
    
    def __store_SE(self,SEID):
        """
        Stores the current Security environment in the secure access module. The
        SEID is used as a reference to identify the SE.
        """
        SEstr = dumps(self.CAPDU_SE)
        try:
            self.SAM.addkey(SEID, SEstr) #TODO: Need SAM reference
        except ValueError:
            raise SwError["ERR_INCORRECTP1P2"]

    
    def __restore_SE(self,SEID):
        """
        Restores a Security Environment from the SAM and replaces the current SE
        with it 
        """
        SEstr = self.SAM.get_key(SEID)
        SE = loads(SEstr)
        if isinstance(SE, SecurityEnvironment):
            self.CAPDU_SE = SE
        else:
            raise ValueError 
    
    def __erase_SE(self,SEID):
        """
        Erases a Security Environment stored under SEID from the SAM
        """
        self.SAM.removeKey(SEID)
    
    def parse_SM_CAPDU(self,CAPDU,header_authentication):
        """
        Frontend for the __parse_SM command. We set the right Security Environment and restore the
        old one, when the command is finished
        """
        
        current_SE = self.current_SE
        self.current_SE = self.CAPDU_SE
        try:
            capdu = self.__parse_SM(CAPDU,header_authentication)
        except SwError, e: #Restore Security Environment
            self.current_SE = current_SE
            raise e

        self.current_SE = current_SE
        return capdu
       
    def __parse_SM(self,CAPDU,header_authentication):
        """
        This methods parses a data field including Secure Messaging objects.
        SM_header indicates wether or not the header of the message shall be authenticated
        It returns an unprotected command APDU
        @param CAPDU: The protected CAPDU to be parsed
        @param header_authentication: Wether or not the header should be included in authentication mechanisms 
        @return: Unprotected command APDU
        """    
        structure = TLVutils.unpack(CAPDU.data)
        return_data = ["",];
        expected = self.current_SE.sm_objects
        
        cla = None
        ins = None
        p1 = None
        p2 = None
        le = None
        
        if header_authentication:
            to_authenticate = inttostring(CAPDU.cla) + inttostring(CAPDU.ins) + inttostring(CAPDU.p1) + inttostring(CAPDU.p2)
            to_authenticate = CryptoUtils.append_padding("DES-CBC", to_authenticate)
        else:
            to_authenticate = ""

        for object in structure:
            tag, length, value = object
            #TODO: Sanity checking
            #if not SM_Class.has_key(tag):
            #    raise ValueError, "unknown Secure messaging tag %#X" % tag
            if tag % 2 == 1: #Include object in checksum calculation
                to_authenticate += inttostring(tag) + inttostring(length) + value
            
            #SM data objects for encapsulating plain values
            if tag in (SM_Class["PLAIN_VALUE_NO_TLV"],SM_Class["PLAIN_VALUE_NO_TLV_ODD"]):
                return_data.append(value) #FIXME: Need TLV coding?
            elif tag in (SM_Class["PLAIN_VALUE_TLV_INCULDING_SM"],SM_Class["PLAIN_VALUE_TLV_INCULDING_SM_ODD"]):
                #Encapsulated SM objects. Parse them
                return_data.append(self.parse_SM_CAPDU(value, header_authentication)) #FIXME: Need to pack value into a dummy CAPDU
            elif tag in (SM_Class["PLAIN_VALUE_TLV_NO_SM"],SM_Class["PLAIN_VALUE_TLV_NO_SM_ODD"]):
                #Encapsulated plaintext BER-TLV objects
                return_data.append(value)
            elif tag in (SM_Class["Ne"],SM_Class["Ne_ODD"]):
                le = value
            elif tag == SM_Class["PLAIN_COMMAND_HEADER"]:
                if len(value) != 8:
                    raise ValueError, "Plain Command Header expected, but received insufficient data"
                else:
                    cla = value[:2]
                    ins = value[2:4]
                    p1 = value[4:6]
                    p2 = value[6:8]

            #SM data objects for confidentiality
            if tag in (SM_Class["CRYPTOGRAM_PLAIN_TLV_INCLUDING_SM"],SM_Class["CRYPTOGRAM_PLAIN_TLV_INCLUDING_SM_ODD"]):
                #The Cryptogram includes SM objects. We decrypt them and parse the objects.
                plain = decipher(tag,0x80,value) #TODO: Need Le = length
                return_data.append(self.parse_SM_CAPDU(plain, header_authentication))
            elif tag in (SM_Class["CRYPTOGRAM_PLAIN_TLV_NO_SM"],SM_Class["CRYPTOGRAM_PLAIN_TLV_NO_SM_ODD"]):
                #The Cryptogram includes BER-TLV enconded plaintext. We decrypt them and return the objects.
                plain = decipher(tag,0x80,value)
                return_data.append(plain)
            elif tag in (SM_Class["CRYPTOGRAM_PADDING_INDICATOR"],SM_Class["CRYPTOGRAM_PADDING_INDICATOR_ODD"]):
                #The first byte of the specified data field indicates the padding to use:
                """
                Value        Meaning
                '00'     No further indication
                '01'     Padding as specified in 6.2.3.1
                '02'     No padding
                '1X'     One to four secret keys for enciphering information, not keys ('X' is a bitmap with any value from '0' to 'F')
                '11'     indicates the first key (e.g., an "even" control word in a pay TV system)
                '12'     indicates the second key (e.g., an "odd" control word in a pay TV system)
                '13'     indicates the first key followed by the second key (e.g., a pair of control words in a pay TV system)
                '2X'     Secret key for enciphering keys, not information ('X' is a reference with any value from '0' to 'F')
                            (e.g., in a pay TV system, either an operational key for enciphering control words, or a management key
                            for enciphering operational keys)
                '3X'     Private key of an asymmetric key pair ('X' is a reference with any value from '0' to 'F')
                '4X'     Password ('X' is a reference with any value from '0' to 'F')
            '80' to '8E' Proprietary
                """
                padding_indicator = stringtoint(value[0])
                sw, plain = self.decipher(tag,0x80,value[1:])
                if sw != 0x9000:
        			raise ValueError
                plain = CryptoUtils.strip_padding(self.CAPDU_SE.ct.algo,plain,padding_indicator)
                return_data.append(plain)

            #SM data objects for authentication 
            if tag == SM_Class["CHECKSUM"]:
                auth = CryptoUtils.append_padding("DES-CBC", to_authenticate)
                sw, checksum = self.compute_cryptographic_checksum(0x8E, 0x80, auth)
                if checksum != value:
                    print "Failed to verify checksum!"
                    raise SwError(SW["ERR_SECMESSOBJECTSINCORRECT"])
            elif tag == SM_Class["DIGITAL_SIGNATURE"]:
                auth = to_authenticate #FIXME: Need padding?
                sw, signature = self.compute_digital_signature(0x9E, 0x9A, auth)
                if signature != value:
                    print "Failed to verify signature!"
                    raise SwEroor(SW["ERR_SECMESSOBJECTSINCORRECT"])
            elif tag in (SM_Class["HASH_CODE"],SM_Class["HASH_CODE_ODD"]):
                sw, hash = self.hash(p1, p2, to_authenticate)
                if hash != value:
                    print "Failed to verify hash!"
                    raise SwEroor(SW["ERR_SECMESSOBJECTSINCORRECT"])
                
            #Check if we just parsed a expected SM Object:
            pos = 0
            while (pos < len (expected)):
                if expected[pos] == tag:
                    expected = expected[:pos-1] + expected[pos:]
                    break
                pos += 1
                
        #Form unprotected CAPDU
        if cla == None:
            cla = CAPDU.cla
        if ins == None:
            ins = CAPDU.ins
        if p1 == None:
            p1 = CAPDU.p1
        if p2 == None:
            p2 = CAPDU.p2
        if le == None:
            le = CAPDU.le
        if expected != "":
            raise SwError(SW["ERR_SECMESSOBJECTSMISSING"])
        
        c = C_APDU(cla=cla,ins=ins,p1=p1,p2=p2,le=le,data="".join(return_data))    
        return c

    def protect_response(self,sw,result):
        """
        Frontend for the __protect command. We set the right Security Environment and restore the
        old one, when the command is finished
        """
        
        current_SE = self.current_SE
        self.current_SE = self.RAPDU_SE
        try:
            sw, data = self.__protect(sw,result)
        except SwError, e: #Restore Security Environment
            self.current_SE = current_SE
            raise e

        self.current_SE = current_SE
        return sw, data

    def __protect(self,sw,result): 
        """
        This method protects a response APDU using secure messanging mechanisms
        It returns the protected data and the SW bytes
        """
        expected = self.current_SE.sm_objects
        for pos in range(len(expected)):
            tag = expected[pos]

        return_data = ""
        #if sw == SW["NORMAL"]:
        #    sw = inttostring(sw)
        #    length = len(sw) 
        #    tag = SM_Class["PLAIN_PROCESSING_STATUS"]
        #    tlv_sw = TLVutils.pack([(tag,length,sw)])
        #    return_data += tlv_sw

        if result != "": # Encrypt the data included in the RAPDU
            sw, encrypted = self.encipher(0x82, 0x80, result)
            encrypted = "\x01" + encrypted
            encrypted_tlv = TLVutils.pack([(SM_Class["CRYPTOGRAM_PADDING_INDICATOR_ODD"],len(encrypted),encrypted)])
            return_data += encrypted_tlv 
        
        if sw == SW["NORMAL"]:
            if self.CAPDU_SE.cct.algo == None:
                raise SwError(SW["CONDITIONSNOTSATISFIED"])
            elif self.CAPDU_SE.cct.algo == "CCT":
                tag = SM_Class["CHECKSUM"]
                to_auth = CryptoUtils.append_padding("DES-ECB", return_data)
                sw, auth = self.compute_cryptographic_checksum(0x8E, 0x80, to_auth)
                length = len(auth)
                return_data += TLVutils.pack([(tag,length,auth)])
            elif self.CAPDU_SE.cct.algo == "SIGNATURE":
                tag = SM_Class["DIGITAL_SIGNATURE"]
                hash = self.hash(0x90, 0x80, return_data)
                sw, auth = self.compute_digital_signature(0x9E, 0x9A, hash)
                length = len(auth)
                return_data += TLVutils.pack([(tag,length,auth)])
        
        return SW["NORMAL"], return_data

    #The following commands implement ISO 7816-8 {{{
    def perform_security_operation(self,p1,p2,data):
        """
        In the end this command is nothing but a big switch for all the other commands
        in ISO 7816-8. It will invoke the appropiate command and return its result
        """
        
        allowed_P1P2 = ((0x90,0x80),(0x90,0xA0),(0x9E,0x9A),(0x9E,0xAC),(0x9E,0xBC),(0x00,0xA2),
                        (0x00,0xA8),(0x00,0x92),(0x00,0xAE),(0x00,0xBE),(0x82,0x80),(0x84,0x80),
                        (0x86,0x80),(0x80,0x82),(0x80,0x84),(0x80,0x86))
    
        if (p1,p2) not in allowed_P1P2:
            raise SwError(SW["INCORRECTP1P2"])
       
        if(p2 in (0x80,0xA0) and p1==0x90):
            sw, response_data = self.hash(p1,p2,data)
        elif(p2 in (0x9A,0xAC,0xBC) and p1 == 0x9E):
            sw, response_data = self.compute_digital_signature(p1, p2, data)
        elif(p2 == 0xA2 and p1 == 0x00):
            sw, response_data = self.verify_cryptographic_checksum(p1, p2, data)
        elif(p2 == 0xA8 and p1 == 0x00):
            sw, response_data = self.verify_digital_signature(p1, p2, data)
        elif(p2 in (0x92,0xAE,0xBE) and p1 == 0x00):
            sw, response_data = self.verify_certificate(p1, p2, data)
        elif (p2 == 0x80 and p1 in (0x82,0x84,0x86)):
            sw, response_data = self.encipher(p1, p2, data)
        elif (p2 in (0x82,0x84,0x86) and p1 == 0x80):
            sw, response_data = self.decipher(p1, p2, data)
        
        if p1 == 0x00:
            assert response_data == ""
        
        return sw, response_data
        
    
    def compute_cryptographic_checksum(self,p1,p2,data):
        """
        Compute a cryptographic checksum (e.g. MAC) for the given data.
        Algorithm and key are specified in the current (CAPDU) SE
        @bug: Always use CAPDU settings ???
        """
        if p1 != 0x8E or p2 != 0x80:
            raise SwError(SW["ERR_INCORRECTP1P2"])
        if self.CAPDU_SE.cct.algo == None or self.current_SE.cct.key == None:
            raise SWError(SE["ERR_CONDITIONNOTSATISFIED"])
         
        checksum = CryptoUtils.crypto_checksum(self.current_SE.cct.algo, 
                                               self.current_SE.cct.key, 
                                               data, 
                                               self.current_SE.cct.iv)
        return SW["NORMAL"], checksum
    
    def compute_digital_signature(self,p1,p2,data):
        """
        Compute a digital signature for the given data.
        Algorithm and key are specified in the current SE
        @param p1: Must be 0x9E = Secure Messaging class for digital signatures
        @param p2: Must be one of 0x9A, 0xAC, 0xBC. Indicates what kind of data is 
                   included in the data field.
        """
        
        if p1 != 0x9E or not p2 in (0x9A,0xAC,0xBC):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        if self.current_SE.dst.key == None:
            raise SWError(SE["ERR_CONDITIONNOTSATISFIED"])
              
        if p2 == 0x9A: #Data to be signed
            to_sign = data
        elif p2 == 0xAC: #Data objects, sign values
            to_sign = ""
            structure = TLVutils.unpack(data)
            for tag, length, value in structure:
                to_sign += value
        elif p2 == 0xBC: #Data objects to be signed
            pass
        
        signature = self.current_SE.dst.key.sign(data,"")
        return SW["NORMAL"], signature
    
    def hash(self,p1,p2,data):
        """
        Hash the given data using the algorithm specified by the
        current Security environment.
        Return raw data (no TLV coding).
        """        
        if p1 != 0x90 or not p2 in (0x80,0xA0):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        algo = self.current_SE.ht.algo
        if algo == None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])
        try:
            hash = CryptoUtils.hash("MD5",data)
        except ValueError: #FIXME: Type of error
            raise SwError(SW["ERR_SECMESSNOTSUPPORTED"])

        return SW["NORMAL"], hash

    def verify_cryptographic_checksum(self,p1,p2,data):
        """
        Verify the cryptographic checksum contained in the data field. Data field must contain
        a cryptographic checksum (tag 0x8E) and a plain value (tag 0x80)
        """
        plain = ""
        cct = ""

        algo = self.current_SE.cct.algo
        key = self.current_SE.cct.key
        iv = self.current_SE.cct.iv
        if algo == None or key == None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        structure = TLVutils.unpack(config)
        for tag, length, value in structure:
            if tag == 0x80:
                plain = value
            elif tag == 0x8E:
                cct = value
        if plain == "" or cct == "":
            raise SwError(SW["ERR_SECMESSOBJECTSMISSING"])
        else:
            my_cct = CryptoUtils.crypto_checksum(algo,key,plain,iv)
            if my_cct == cct:
                return SW["NORMAL"], ""
            else:
                raise SwError["ERR_SECMESSOBJECTSINCORRECT"]

    def verify_digital_signature(self,p1,p2,data):
        """
        Verify the digital signature contained in the data field. Data must contain
        a data to sign (tag 0x9A, 0xAC or 0xBC) and a digital signature (0x9E)
        """
        key = self.current_SE.dst.key
        to_sign = ""
        signature = ""

        if key == None:
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        structure = TLVutils.unpack(config)
        for tag, length, value in structure:
            if tag == 0x9E:
                signature = value
            elif tag == 0x9A: #FIXME: Correct treatment of all possible tags
                to_sign = value
            elif tag == 0xAC:
                pass
            elif tag == 0xBC:
                pass

        if to_sign == "" or signature == "":
            raise SwError(SW["ERR_SECMESSOBJECTSMISSING"])

        my_signature = key.sign(value)
        if my_signature == signature:
            return SW["NORMAL"], ""
        else:
            raise SwError(["ERR_SECMESSOBJECTSINCORRECT"])

    def verify_certificate(self,p1,p2,data):
        if p1 != 0x00 or p2 not in (0x92,0xAE,0xBE):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        else:
            raise NotImplementedError

    def encipher(self,p1,p2,data):
        """
        Encipher data using key, algorithm, IV and Padding specified
        by the current Security environment.
        Return raw data (no TLV coding).
        """
        algo = self.current_SE.ct.algo
        key = self.current_SE.ct.key
        if key == None or algo == None:
            return SW["ERR_CONDITIONNOTSATISFIED"], ""
        else:
            padded = CryptoUtils.append_padding(algo, data)
            crypted = CryptoUtils.cipher(True, algo, key, padded, self.current_SE.ct.iv)
            return SW["NORMAL"], crypted

    def decipher(self,p1,p2,data):
        """
        Decipher data using key, algorithm, IV and Padding specified
        by the current Security environment.
        Return raw data (no TLV coding). Padding is not removed!!!
        """
        algo = self.current_SE.ct.algo
        key = self.current_SE.ct.key
        if key == None or algo == None:
            return SW["ERR_CONDITIONNOTSATISFIED"], ""
        else:
            plain = CryptoUtils.cipher(False,algo,key,data,self.current_SE.ct.iv)
            return SW["NORMAL"], plain

    def generate_public_key_pair(self,p1,p2,data):
        """
        Generate a new public-private key pair.
        """
        from Crypto.PublicKey import RSA, DSA
        from Crypto.Util.randpool import RandomPool
        rnd = RandomPool()

        cipher = self.CAPDU_SE.ct.algo #FIXME: Current SE?

        c_class = locals().get(cipher, None)
        if c_class is None: 
            raise SwError(SW["ERR_CONDITIONNOTSATISFIED"])

        if p1 & 0x01 == 0x00: #Generate key
            PublicKey = c_class.generate(self.CAPDU_SE.dst.keylength,rnd.get_bytes)
            self.CAPDU_SE.dst.key = PublicKey
        else:
            pass #Read key

        #Encode keys
        if cipher == "RSA":
            #Public key
            n = str(PublicKey.__getstate__()['n'])
            e = str(PublicKey.__getstate__()['e'])
            pk = ((0x81,len(n),n),(0x82,len(e),e))
            result = TLVutils.bertlv_pack(pk)
            #result = TLVutils.bertlv_pack((0x7F49,len(pk),pk))
            #Private key
            d = PublicKey.__getstate__()['d']
        elif cipher == "DSA":
            #Public key
            p = str(PublicKey.__getstate__()['p'])
            q = str(PublicKey.__getstate__()['q'])
            g = str(PublicKey.__getstate__()['g'])
            y = str(PublicKey.__getstate__()['y'])
            #Private key
            x = str(PublicKey.__getstate__()['x'])
        #Add more algorithms here

        if p1 & 0x02 == 0x02:
            return SW["NORMAL"], result
        else:
            #FIXME: Where to put the keys?
            return SW["NORMAL"], ""

    #}}}
class CryptoflexSM(Secure_Messaging):
    def __init__(self,mf):
        Secure_Messaging.__init__(self,mf) #Does Cryptoflex need its own SE?

    def generate_public_key_pair(self,p1,p2,data):
        """
        In the Cryptoflex card this command only supports RSA keys.

        @param data: Contains the public exponent used for key generation
        @param p1: The keynumber. Can be used later to refer to the generated key
        @param p2: Used to specifiy the keylength. The mapping is: 0x40 => 256 Bit, 0x60 => 512 Bit, 0x80 => 1024
        """
        from Crypto.PublicKey import RSA
        from Crypto.Util.randpool import RandomPool

        keynumber = p1 #TODO: Check if key exists

        keylength_dict = {0x40: 256, 0x60: 512, 0x80: 1024}

        if not keylength_dict.has_key(p2):
            raise SwError(SW["ERR_INCORRECTP1P2"])
        else:
            keylength = keylength_dict[p2]

        rnd = RandomPool()
        PublicKey = RSA.generate(keylength,rnd.get_bytes)
        self.CAPDU_SE.dst.key = PublicKey

        e_in = struct.unpack("<i",data)
        if e_in[0] != 65537:
            print "Warning: Exponents different from 65537 are ignored! The Exponent given is %i" % e_in[0]

        #Encode Public key
        n = PublicKey.__getstate__()['n']
        n_str = inttostring(n)
        n_str = n_str[::-1]
        e = PublicKey.__getstate__()['e']
        e_str = inttostring(e,4)
        e_str = e_str[::-1]
        padd = 187 * '\x30' #We don't have chinese remainder theorem components, so we need to inject padding
        pk_n = TLVutils.bertlv_pack(((0x81,len(n_str),n_str),(0x01,len(padd),padd),(0x82,len(e_str),e_str)))
        #Private key
        d = PublicKey.__getstate__()['d']

        #Write result to FID 10 12 EF-PUB-KEY
        df = self.mf.currentDF()
        file = df.select("fid", 0x1012)
        file.writebinary([0], [pk_n])
        data = file.getenc('data')
        
        #Write private key to FID 00 12 EF-PRI-KEY (not necessary?)      
        file = df.select("fid", 0x0012)
        file.writebinary([0], [inttostring(d)]) #Do we need encoding for the private key?
        data = file.getenc('data')     
        return PublicKey           

class ePass_SM(Secure_Messaging):
    
    def __init__(self,MF,CAPDU_SE,RAPDU_SE,ssc=None):
        self.ssc = ssc
        Secure_Messaging.__init__(self, MF, CAPDU_SE, RAPDU_SE)
           
    def compute_cryptographic_checksum(self,p1,p2,data):
        """
        Compute a cryptographic checksum (e.g. MAC) for the given data.
        Algorithm and key are specified in the current (CAPDU) SE. The ePass
        uses a Send Sequence Counter for MAC calculation
        @bug: Always use CAPDU settings ???
        """
        if p1 != 0x8E or p2 != 0x80:
            raise SwError(SW["ERR_INCORRECTP1P2"])
        
        #checksum = CryptoUtils.calculate_MAC(self.CAPDU_SE.cct.key,data,self.CAPDU_SE.cct.iv)
        self.ssc += 1
        checksum = CryptoUtils.crypto_checksum(self.CAPDU_SE.cct.algo,
                                               self.CAPDU_SE.cct.key,
                                               data,
                                               self.CAPDU_SE.cct.iv,
                                               self.ssc)

        return SW["NORMAL"], checksum

if __name__ == "__main__":
    """
    Unit test:
    """

    password = "DUMMYKEYDUMMYKEY"
    #generate_SAM_config("1234567890", "1234", "keykeykeykeykeyk", path, password)

    MyCard = SAM("1234", "1234567890")
    try:
        print MyCard.verify(0x00, 0x00, "5678")
    except SwError, e:
        print e.message

    print "Counter = " + str(MyCard.counter)
    print MyCard.verify(0x00,  0x00, "1234")
    print "Counter = " + str(MyCard.counter)
    sw, challenge = MyCard.get_challenge(0x00, 0x00, "")
    print "Before encryption: " + challenge
    padded = CryptoUtils.append_padding("DES3-ECB", challenge)
    sw, result = MyCard.internal_authenticate(0x00, 0x00, padded)
    print "Internal Authenticate status code: %x" % sw

    try:
        sw, res = MyCard.external_authenticate(0x00, 0x00, result)
    except SwError, e:
        print e.message
        sw = e.sw
    print "Decryption Status code: %x" % sw

    SM = Secure_Messaging(None)
    testvektor = "foobar"
    print "Testvektor = %s" % testvektor
    sw, hash = SM.hash(0x90,0x80,testvektor)
    print "SW after hashing = %s" % sw
    print "Hash = %s" % hash
    sw, crypted = SM.encipher(0x00, 0x00, testvektor)
    print "SW after encryption = %s" % sw
    sw, plain = SM.decipher(0x00, 0x00, crypted)
    print "SW after encryption = %s" % sw
    print "Testvektor after en- and deciphering: %s" % plain
    sw, pk = SM.generate_public_key_pair(0x02, 0x00, "")
    print "SW after keygen = %s" % sw
    print "Public Key = %s" % pk
    CF = CryptoflexSM(None)
    print CF.generate_public_key_pair(0x00, 0x80, "\x01\x00\x01\x00")
    print MyCard._read_FS_data(0x01, 16)
    print MyCard._get_referenced_key(0x01)
