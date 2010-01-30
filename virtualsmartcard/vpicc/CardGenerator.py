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

import sys, getpass, anydbm
from pickle import loads, dumps
from TLVutils import pack
from utils import inttostring
from SmartcardFilesystem import MF, TransparentStructureEF 

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

def generate_iso_card():
    from SmartcardSAM import SAM
     
    print "Using default SAM. Insecure!!!"
    sam = SAM("1234", "1234567890") #FIXME: Use user provided data
    
    mf = MF(filedescriptor=FDB["DF"], lifecycle=LCB["ACTIVATED"], dfname=None)
    self.SAM.set_MF(self.mf)
    
    return mf, sam

def generate_ePass():
        from PIL import Image
        from SmartcardFilesystem import DF 
        from SmartcardSAM import PassportSAM
        
        MRZ1 = "P<UTOERIKSSON<<ANNA<MARIX<<<<<<<<<<<<<<<<<<<"
        MRZ2 = "L898902C<3UTO6908061F9406236ZE184226B<<<<<14"
        MRZ = MRZ1 + MRZ2        

        picturepath = "jp2.jpg"
        try:
            im = Image.open(picturepath)
            pic_width, pic_height = im.size
            fd = open(picturepath,"rb")
            picture = fd.read()
            fd.close()
        except IOError:
            print "Could not find picture %s" % picturepath
            pic_width = 0
            pic_height = 0
            picture = None  

        mf = MF()
        
        #We need a MF with Application DF \xa0\x00\x00\x02G\x10\x01
        mf.append(DF(parent=mf, fid=4, dfname='\xa0\x00\x00\x02G\x10\x01', bertlv_data=[]))
        df = mf.currentDF()
        mf.append(TransparentStructureEF(parent=df, fid=0x011E, filedescriptor=0, data=""))#EF.COM
        mf.append(TransparentStructureEF(parent=df, fid=0x0101, filedescriptor=0, data=""))#EF.DG1
        mf.append(TransparentStructureEF(parent=df, fid=0x0102, filedescriptor=0, data=""))#EF.DG2
        mf.append(TransparentStructureEF(parent=df, fid=0x010D, filedescriptor=0, data=""))#EF.SOD
        #EF.COM
        COM = pack([(0x5F01,4,"0107"),(0x5F36,6,"040000"),(0x5C,2,"6175")])
        COM = pack(((0x60,len(COM),COM),))
        fid = df.select("fid",0x011E)
        fid.writebinary([0],[COM])        
        #EF.DG1
        DG1 = pack([(0x5F1F,len(MRZ),MRZ)])
        DG1 = pack([(0x61,len(DG1),DG1)])
        fid = df.select("fid",0x0101)
        fid.writebinary([0],[DG1])        
        #EF.DG2
        if picture != None:
            IIB = "\x00\x01" + inttostring(pic_width,2) + inttostring(pic_height,2) + 6 * "\x00" 
            length = 32 + len(picture) #32 is the length of IIB + FIB
            FIB = inttostring(length,4) + 16 * "\x00"
            FRH = "FAC" + "\x00" + "010" + "\x00" + inttostring(14+length,4) + inttostring(1,2)
            picture = FRH + FIB + IIB + picture
            DG2 = pack([(0xA1,8,"\x87\x02\x01\x01\x88\x02\x05\x01"),(0x5F2E,len(picture),picture)])
            DG2 = pack([(0x02,1,"\x01"),(0x7F60,len(DG2),DG2)])
            DG2 = pack([(0x7F61,len(DG2),DG2)])
            fid = df.select("fid",0x0102)
            fid.writebinary([0],[DG2])

        sam = PassportSAM(mf)
        return mf, sam

def generate_cryptoflex():
    from SmartcardFilesystem import CryptoflexMF
    from SmartcardSAM import CryptoflexSAM
                    
    mf = CryptoflexMF()
    mf.append(TransparentStructureEF(parent=mf, fid=0x0002, filedescriptor=0x01,
                                     data="\x00\x00\x00\x01\x00\x01\x00\x00")) #EF.ICCSN
    sam = CryptoflexSAM(mf)
    
    return mf, sam

def loadCard(filename, password=None):
    from CryptoUtils import read_protected_string
    
    try:
        db = anydbm.open(filename, 'r')
    except anydbm.error:
        print "Failed to open " + filename
    
    if password is None:
        password = getpass.getpass("Please enter your password.")
    
    serializedMF = read_protected_string(db["mf"], password)
    serializedSAM = read_protected_string(db["sam"], password)
    SAM = loads(serializedSAM)
    MF = loads(serializedMF)
    #self.type = db["type"]
    
    return SAM, MF

def saveCard(mf, sam, filename, password=None):      
    from CryptoUtils import protect_string        
    
    if filename != None:
        print "Saving smartcard configuration to %s" % filename
    else: #TODO: Ask user for filename 
        pass
    
    if password is None:
        passwd1 = getpass.getpass("Please enter a password.")
        passwd2 = getpass.getpass("Please retype your password.")
        if (passwd1 != passwd2):
            raise ValueError, "Passwords did not match. Will now exit"
        else:
            password = passwd1
    
    mf_string = dumps(mf)
    sam_string = dumps(sam)
    protectedMF = protect_string(mf_string, password)
    protectedSAM = protect_string(sam_string, password)
    
    try:
        db = anydbm.open(filename, 'c')
        db["mf"] = protectedMF
        db["sam"] = protectedSAM
        #db["type"] = self.type
        db.close()
    except anydbm.error:
        print "Failed to write data to disk"
                          
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
        print "You have to provide a filename using the -f option"
        sys.exit()
    
    if options.type == 'iso7816':
        mf, sam = generate_iso_card()
    elif options.type == 'ePass':
        mf, sam = generate_ePass()
    elif options.type == 'cryptoflex':
        mf, sam = generate_cryptoflex()
        
    saveCard(mf, sam, options.filename)