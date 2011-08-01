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

import sys, getpass, anydbm, readline, logging
from pickle import loads, dumps
from virtualsmartcard.TLVutils import pack
from virtualsmartcard.utils import inttostring
from virtualsmartcard.SmartcardFilesystem import MF, DF, TransparentStructureEF
from virtualsmartcard.ConstantDefinitions import FDB
from virtualsmartcard.CryptoUtils import protect_string, read_protected_string
import virtualsmartcard.SmartcardSAM

# pgp directory
#self.mf.append(DF(parent=self.mf,
    #fid=4, dfname='\xd2\x76\x00\x01\x24\x01', bertlv_data=[]))
# pkcs-15 directories
#self.mf.append(DF(parent=self.mf,
    #fid=1, dfname='\xa0\x00\x00\x00\x01'))
#self.mf.append(DF(parent=self.mf,
    #fid=2, dfname='\xa0\x00\x00\x03\x08\x00\x00\x10\x00'))
#self.mf.append(DF(parent=self.mf,
    #fid=3, dfname='\xa0\x00\x00\x03\x08\x00\x00\x10\x00\x01\x00'))  

class CardGenerator(object):
    
    def __init__(self, card_type=None, sam=None, mf=None):
        self.type = card_type
        self.mf = mf
        self.sam = sam
    
    def __generate_iso_card(self):
        default_pin = "1234"
        default_cardno = "1234567890"
        
        logging.warning("Using default SAM parameters. PIN=%s, Card Nr=%s"
                        & (default_pin, default_cardno))
        #TODO: Use user provided data
        self.sam = virtualsmartcard.SmartcardSAM.SAM(default_pin, default_cardno)
        
        self.mf = MF(filedescriptor=FDB["DF"])
        self.sam.set_MF(self.mf)
           
    def __generate_ePass(self):
        from PIL import Image
            
        MRZ = raw_input("Please enter the MRZ as one string: ") #TODO: Sanity checks

        readline.set_completer_delims("")
        readline.parse_and_bind("tab: complete")
        
        picturepath = raw_input("Please enter the path to an image: ")
        picturepath = picturepath.strip()
        
        #MRZ1 = "P<UTOERIKSSON<<ANNA<MARIX<<<<<<<<<<<<<<<<<<<"
        #MRZ2 = "L898902C<3UTO6908061F9406236ZE184226B<<<<<14"
        #MRZ = MRZ1 + MRZ2        

        try:
            im = Image.open(picturepath)
            pic_width, pic_height = im.size
            fd = open(picturepath,"rb")
            picture = fd.read()
            fd.close()
        except IOError:
            logging.warning("Failed to open file: " + picturepath)
            pic_width = 0
            pic_height = 0
            picture = None  

        mf = MF()
        
        #We need a MF with Application DF \xa0\x00\x00\x02G\x10\x01
        df = DF(parent=mf, fid=4, dfname='\xa0\x00\x00\x02G\x10\x01',
                bertlv_data=[])

        #EF.COM
        COM = pack([(0x5F01, 4, "0107"), (0x5F36, 6, "040000"),
                    (0x5C, 2, "6175")])
        COM = pack(((0x60, len(COM), COM),))
        df.append(TransparentStructureEF(parent=df, fid=0x011E,
                  filedescriptor=0, data=COM))

        #EF.DG1
        DG1 = pack([(0x5F1F, len(MRZ), MRZ)])
        DG1 = pack([(0x61, len(DG1), DG1)])
        df.append(TransparentStructureEF(parent=df, fid=0x0101,
                  filedescriptor=0, data=DG1))

        #EF.DG2
        if picture != None:
            IIB = "\x00\x01" + inttostring(pic_width, 2) +\
                    inttostring(pic_height, 2) + 6 * "\x00" 
            length = 32 + len(picture) #32 is the length of IIB + FIB
            FIB = inttostring(length, 4) + 16 * "\x00"
            FRH = "FAC" + "\x00" + "010" + "\x00" +\
                    inttostring(14 + length, 4) + inttostring(1, 2)
            picture = FRH + FIB + IIB + picture
            DG2 = pack([(0xA1, 8, "\x87\x02\x01\x01\x88\x02\x05\x01"), 
                    (0x5F2E, len(picture), picture)])
            DG2 = pack([(0x02, 1, "\x01"), (0x7F60, len(DG2), DG2)])
            DG2 = pack([(0x7F61, len(DG2), DG2)])
        else:
            DG2 = ""
        df.append(TransparentStructureEF(parent=df, fid=0x0102,
                  filedescriptor=0, data=DG2))

        #EF.SOD
        df.append(TransparentStructureEF(parent=df, fid=0x010D,
                  filedescriptor=0, data=""))

        mf.append(df)

        self.mf = mf
        self.sam = virtualsmartcard.SmartcardSAM.PassportSAM(self.mf)
    
    def __generate_cryptoflex(self):
        from virtualsmartcard.SmartcardFilesystem import CryptoflexMF
                       
        self.mf = CryptoflexMF()
        self.mf.append(TransparentStructureEF(parent=self.mf, fid=0x0002,
                       filedescriptor=0x01,
                       data="\x00\x00\x00\x01\x00\x01\x00\x00")) #EF.ICCSN
        self.sam = virtualsmartcard.SmartcardSAM.CryptoflexSAM(self.mf)
        
    def generateCard(self):
        """Generate a new card"""
        if self.type == 'iso7816':
            self.__generate_iso_card()
        elif self.type == 'ePass':
            self.__generate_ePass()
        elif self.type == 'cryptoflex':
            self.__generate_cryptoflex()
        else:
            return (None, None)
    
    def getCard(self):
        """Get the MF and SAM from the current card"""
        if self.sam is None or self.mf is None:
            self.generateCard()
        return self.mf, self.sam
    
    def setCard(self, mf=None, sam=None):
        """Set the MF and SAM of the current card"""
        if mf != None:
            self.mf = mf
        if sam != None:
            self.sam = sam
        
    
    def loadCard(self, filename, password=None):
        """Load a card from disk"""
        db = anydbm.open(filename, 'r')
        
        if password is None:
            password = getpass.getpass("Please enter your password:")
        
        serializedMF = read_protected_string(db["mf"], password)
        serializedSAM = read_protected_string(db["sam"], password)
        self.sam = loads(serializedSAM)
        self.mf = loads(serializedMF)
        self.type = db["type"]
            
    def saveCard(self, filename, password=None):
        """Save the currently running card to disk"""
        if password is None:
            passwd1 = getpass.getpass("Please enter your password:")
            passwd2 = getpass.getpass("Please retype your password:")
            if (passwd1 != passwd2):
                raise ValueError, "Passwords did not match. Will now exit"
            else:
                password = passwd1
        
        if self.mf == None or self.sam == None:
            raise ValueError, "Card Generator wasn't set up properly" +\
                 "(missing MF or SAM)."
        
        mf_string = dumps(self.mf)
        sam_string = dumps(self.sam)
        protectedMF = protect_string(mf_string, password)
        protectedSAM = protect_string(sam_string, password)
        
        db = anydbm.open(filename, 'c')
        db["mf"] = protectedMF
        db["sam"] = protectedSAM
        db["type"] = self.type
        db["version"] = "0.1"
        db.close()

                          
if __name__ == "__main__":
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("-t", "--type", action="store", type="choice",
            default='iso7816',
            choices=['iso7816', 'cryptoflex', 'ePass'],
            help="Type of Smartcard [default: %default]")
    parser.add_option("-f", "--file", action="store", type="string",
            dest="filename", default=None,
            help="Where to save the generated card")
    (options, args) = parser.parse_args()

    if options.filename == None:
        logging.error("You have to provide a filename using the -f option")
        sys.exit()
    
    generator = CardGenerator(options.type)
    generator.generateCard()
    generator.saveCard(options.filename)
    
