#
# Copyright (C) 2011 Dominik Oepen
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

import binascii
import getpass
import logging
try:
    import readline
except ImportError:
    import pyreadline as readline
import sys

from pickle import loads, dumps
from virtualsmartcard.TLVutils import pack
from virtualsmartcard.utils import inttostring
from virtualsmartcard.SmartcardFilesystem import MF, DF, TransparentStructureEF
from virtualsmartcard.ConstantDefinitions import FDB, ALGO_MAPPING
from virtualsmartcard.SmartcardSAM import SAM

# pgp directory
# self.mf.append(DF(parent=self.mf,
#                   fid=4, dfname='\xd2\x76\x00\x01\x24\x01', bertlv_data=[]))
# pkcs-15 directories
# self.mf.append(DF(parent=self.mf,
#                fid=1, dfname='\xa0\x00\x00\x00\x01'))
# self.mf.append(DF(parent=self.mf,
#                fid=2, dfname='\xa0\x00\x00\x03\x08\x00\x00\x10\x00'))
# self.mf.append(DF(parent=self.mf,
#                fid=3, dfname='\xa0\x00\x00\x03\x08\x00\x00\x10\x00\x01\x00'))


class CardGenerator(object):
    """This class is used to generate the SAM and filesystem for the
    different supported card types. It is also able used for persistent storage
    (in encrypted form) of the card on disks. """

    def __init__(self, card_type=None, sam=None, mf=None):
        self.type = card_type
        self.mf = mf
        self.sam = sam
        self.password = None
        self.datagroups = {}

    def __generate_iso_card(self):
        default_pin = "1234"
        default_cardno = "1234567890"

        logging.warning("Using default SAM parameters. PIN=%s, Card Nr=%s"
                        % (default_pin, default_cardno))
        # TODO: Use user provided data
        self.sam = SAM(default_pin, default_cardno)

        self.mf = MF(filedescriptor=FDB["DF"])
        self.sam.set_MF(self.mf)

    def __generate_ePass(self):
        """Generate the MF and SAM of an ICAO passport. This method is
        responsible for generating the filesystem and filling it with content.
        Therefore it must interact with the user by prompting for the MRZ and
        optionally for the path to a photo."""
        from PIL import Image
        from virtualsmartcard.cards.ePass import PassportSAM

        # TODO: Sanity checks
        MRZ = raw_input("Please enter the MRZ as one string: ")

        readline.set_completer_delims("")
        readline.parse_and_bind("tab: complete")

        picturepath = raw_input("Please enter the path to an image: ")
        picturepath = picturepath.strip()

        # MRZ1 = "P<UTOERIKSSON<<ANNA<MARIX<<<<<<<<<<<<<<<<<<<"
        # MRZ2 = "L898902C<3UTO6908061F9406236ZE184226B<<<<<14"
        # MRZ = MRZ1 + MRZ2

        try:
            im = Image.open(picturepath)
            pic_width, pic_height = im.size
            fd = open(picturepath, "rb")
            picture = fd.read()
            fd.close()
        except IOError:
            logging.warning("Failed to open file: " + picturepath)
            pic_width = 0
            pic_height = 0
            picture = None

        mf = MF()

        # We need a MF with Application DF \xa0\x00\x00\x02G\x10\x01
        df = DF(parent=mf, fid=4, dfname='\xa0\x00\x00\x02G\x10\x01',
                bertlv_data=[])

        # EF.COM
        COM = pack([(0x5F01, 4, "0107"), (0x5F36, 6, "040000"),
                    (0x5C, 2, "6175")])
        COM = pack(((0x60, len(COM), COM),))
        df.append(TransparentStructureEF(parent=df, fid=0x011E,
                  filedescriptor=0, data=COM))

        # EF.DG1
        DG1 = pack([(0x5F1F, len(MRZ), MRZ)])
        DG1 = pack([(0x61, len(DG1), DG1)])
        df.append(TransparentStructureEF(parent=df, fid=0x0101,
                  filedescriptor=0, data=DG1))

        # EF.DG2
        if picture is not None:
            IIB = "\x00\x01" + inttostring(pic_width, 2) +\
                    inttostring(pic_height, 2) + 6 * "\x00"
            length = 32 + len(picture)  # 32 is the length of IIB + FIB
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

        # EF.SOD
        df.append(TransparentStructureEF(parent=df, fid=0x010D,
                  filedescriptor=0, data=""))

        mf.append(df)

        self.mf = mf
        self.sam = PassportSAM(self.mf)

    def __generate_nPA(self):
        from virtualsmartcard.cards.nPA import nPA_SAM
        mappings = [
            ('\x04\x00\x7f\x00\x07\x02\x02\x04', range(1, 5), range(1, 5),
             "PACE"),
            ('\x04\x00\x7f\x00\x07\x02\x02\x02', range(1, 3), range(1, 6),
             "TA"),
            ('\x04\x00\x7f\x00\x07\x02\x02\x02', [1], [6], "TA"),
            ('\x04\x00\x7f\x00\x07\x02\x02\x03', range(1, 3), range(1, 5),
             "CA"),
            ('\x04\x00\x7f\x00\x07\x02\x02\x05', range(1, 3), range(1, 6),
             "RI"),
        ]
        for oid_base, x_list, y_list, algo in mappings:
            for oid in [oid_base + chr(x) + chr(y) for x in x_list for y in
                        y_list]:
                ALGO_MAPPING[oid] = algo
        ALGO_MAPPING["\x04\x00\x7f\x00\x07\x03\x01\x04\x03"] = "CommunityID"
        ALGO_MAPPING["\x04\x00\x7f\x00\x07\x03\x01\x04\x02"] = "DateOfExpiry"
        ALGO_MAPPING["\x04\x00\x7f\x00\x07\x03\x01\x04\x01"] = "DateOfBirth"

        self.mf = MF()

        card_access = "\x31\x81\xb3\x30\x0d\x06\x08\x04\x00\x7f\x00\x07\x02"\
                      "\x02\x02\x02\x01\x02\x30\x12\x06\x0a\x04\x00\x7f\x00"\
                      "\x07\x02\x02\x03\x02\x02\x02\x01\x02\x02\x01\x41\x30"\
                      "\x12\x06\x0a\x04\x00\x7f\x00\x07\x02\x02\x03\x02\x02"\
                      "\x02\x01\x02\x02\x01\x45\x30\x12\x06\x0a\x04\x00\x7f"\
                      "\x00\x07\x02\x02\x04\x02\x02\x02\x01\x02\x02\x01\x0d"\
                      "\x30\x1c\x06\x09\x04\x00\x7f\x00\x07\x02\x02\x03\x02"\
                      "\x30\x0c\x06\x07\x04\x00\x7f\x00\x07\x01\x02\x02\x01"\
                      "\x0d\x02\x01\x41\x30\x1c\x06\x09\x04\x00\x7f\x00\x07"\
                      "\x02\x02\x03\x02\x30\x0c\x06\x07\x04\x00\x7f\x00\x07"\
                      "\x01\x02\x02\x01\x0d\x02\x01\x45\x30\x2a\x06\x08\x04"\
                      "\x00\x7f\x00\x07\x02\x02\x06\x16\x1e\x68\x74\x74\x70"\
                      "\x3a\x2f\x2f\x62\x73\x69\x2e\x62\x75\x6e\x64\x2e\x64"\
                      "\x65\x2f\x63\x69\x66\x2f\x6e\x70\x61\x2e\x78\x6d\x6c"
        self.mf.append(TransparentStructureEF(parent=self.mf, fid=0x011c,
                       shortfid=0x1c, data=card_access))
        card_security = "\x30\x82\x05\xA0\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01"\
                        "\x07\x02\xA0\x82\x05\x91\x30\x82\x05\x8D\x02\x01\x03"\
                        "\x31\x0F\x30\x0D\x06\x09\x60\x86\x48\x01\x65\x03\x04"\
                        "\x02\x01\x05\x00\x30\x82\x01\x48\x06\x08\x04\x00\x7F"\
                        "\x00\x07\x03\x02\x01\xA0\x82\x01\x3A\x04\x82\x01\x36"\
                        "\x31\x82\x01\x32\x30\x0D\x06\x08\x04\x00\x7F\x00\x07"\
                        "\x02\x02\x02\x02\x01\x02\x30\x12\x06\x0A\x04\x00\x7F"\
                        "\x00\x07\x02\x02\x03\x02\x02\x02\x01\x02\x02\x01\x41"\
                        "\x30\x12\x06\x0A\x04\x00\x7F\x00\x07\x02\x02\x04\x02"\
                        "\x02\x02\x01\x02\x02\x01\x0D\x30\x17\x06\x0A\x04\x00"\
                        "\x7F\x00\x07\x02\x02\x05\x02\x03\x30\x09\x02\x01\x01"\
                        "\x02\x01\x43\x01\x01\xFF\x30\x17\x06\x0A\x04\x00\x7F"\
                        "\x00\x07\x02\x02\x05\x02\x03\x30\x09\x02\x01\x01\x02"\
                        "\x01\x44\x01\x01\x00\x30\x19\x06\x09\x04\x00\x7F\x00"\
                        "\x07\x02\x02\x05\x02\x30\x0C\x06\x07\x04\x00\x7F\x00"\
                        "\x07\x01\x02\x02\x01\x0D\x30\x1C\x06\x09\x04\x00\x7F"\
                        "\x00\x07\x02\x02\x03\x02\x30\x0C\x06\x07\x04\x00\x7F"\
                        "\x00\x07\x01\x02\x02\x01\x0D\x02\x01\x41\x30\x2A\x06"\
                        "\x08\x04\x00\x7F\x00\x07\x02\x02\x06\x16\x1E\x68\x74"\
                        "\x74\x70\x3A\x2F\x2F\x62\x73\x69\x2E\x62\x75\x6E\x64"\
                        "\x2E\x64\x65\x2F\x63\x69\x66\x2F\x6E\x70\x61\x2E\x78"\
                        "\x6D\x6C\x30\x62\x06\x09\x04\x00\x7F\x00\x07\x02\x02"\
                        "\x01\x02\x30\x52\x30\x0C\x06\x07\x04\x00\x7F\x00\x07"\
                        "\x01\x02\x02\x01\x0D\x03\x42\x00\x04\x92\x5D\xB4\xE1"\
                        "\x7A\xDE\x58\x20\x9F\x96\xFA\xA0\x7F\x1F\x8A\x22\x3F"\
                        "\x82\x3F\x96\xCC\x5D\x78\xCB\xEF\x5D\x17\x42\x20\x88"\
                        "\xFD\xD5\x8E\x56\xBC\x42\x50\xDE\x33\x46\xB3\xC8\x32"\
                        "\xCA\xE4\x86\x35\xFB\x6C\x43\x78\x9D\xE8\xB3\x10\x2F"\
                        "\x43\x93\xB4\x18\xE2\x4A\x13\xD9\x02\x01\x41\xA0\x82"\
                        "\x03\x1C\x30\x82\x03\x18\x30\x82\x02\xBC\xA0\x03\x02"\
                        "\x01\x02\x02\x02\x01\x5C\x30\x0C\x06\x08\x2A\x86\x48"\
                        "\xCE\x3D\x04\x03\x02\x05\x00\x30\x4F\x31\x0B\x30\x09"\
                        "\x06\x03\x55\x04\x06\x13\x02\x44\x45\x31\x0D\x30\x0B"\
                        "\x06\x03\x55\x04\x0A\x0C\x04\x62\x75\x6E\x64\x31\x0C"\
                        "\x30\x0A\x06\x03\x55\x04\x0B\x0C\x03\x62\x73\x69\x31"\
                        "\x0C\x30\x0A\x06\x03\x55\x04\x05\x13\x03\x30\x31\x33"\
                        "\x31\x15\x30\x13\x06\x03\x55\x04\x03\x0C\x0C\x63\x73"\
                        "\x63\x61\x2D\x67\x65\x72\x6D\x61\x6E\x79\x30\x1E\x17"\
                        "\x0D\x31\x30\x31\x30\x30\x35\x31\x31\x30\x37\x35\x32"\
                        "\x5A\x17\x0D\x32\x31\x30\x34\x30\x35\x30\x38\x33\x39"\
                        "\x34\x37\x5A\x30\x47\x31\x0B\x30\x09\x06\x03\x55\x04"\
                        "\x06\x13\x02\x44\x45\x31\x1D\x30\x1B\x06\x03\x55\x04"\
                        "\x0A\x0C\x14\x42\x75\x6E\x64\x65\x73\x64\x72\x75\x63"\
                        "\x6B\x65\x72\x65\x69\x20\x47\x6D\x62\x48\x31\x0C\x30"\
                        "\x0A\x06\x03\x55\x04\x05\x13\x03\x30\x37\x34\x31\x0B"\
                        "\x30\x09\x06\x03\x55\x04\x03\x0C\x02\x44\x53\x30\x82"\
                        "\x01\x13\x30\x81\xD4\x06\x07\x2A\x86\x48\xCE\x3D\x02"\
                        "\x01\x30\x81\xC8\x02\x01\x01\x30\x28\x06\x07\x2A\x86"\
                        "\x48\xCE\x3D\x01\x01\x02\x1D\x00\xD7\xC1\x34\xAA\x26"\
                        "\x43\x66\x86\x2A\x18\x30\x25\x75\xD1\xD7\x87\xB0\x9F"\
                        "\x07\x57\x97\xDA\x89\xF5\x7E\xC8\xC0\xFF\x30\x3C\x04"\
                        "\x1C\x68\xA5\xE6\x2C\xA9\xCE\x6C\x1C\x29\x98\x03\xA6"\
                        "\xC1\x53\x0B\x51\x4E\x18\x2A\xD8\xB0\x04\x2A\x59\xCA"\
                        "\xD2\x9F\x43\x04\x1C\x25\x80\xF6\x3C\xCF\xE4\x41\x38"\
                        "\x87\x07\x13\xB1\xA9\x23\x69\xE3\x3E\x21\x35\xD2\x66"\
                        "\xDB\xB3\x72\x38\x6C\x40\x0B\x04\x39\x04\x0D\x90\x29"\
                        "\xAD\x2C\x7E\x5C\xF4\x34\x08\x23\xB2\xA8\x7D\xC6\x8C"\
                        "\x9E\x4C\xE3\x17\x4C\x1E\x6E\xFD\xEE\x12\xC0\x7D\x58"\
                        "\xAA\x56\xF7\x72\xC0\x72\x6F\x24\xC6\xB8\x9E\x4E\xCD"\
                        "\xAC\x24\x35\x4B\x9E\x99\xCA\xA3\xF6\xD3\x76\x14\x02"\
                        "\xCD\x02\x1D\x00\xD7\xC1\x34\xAA\x26\x43\x66\x86\x2A"\
                        "\x18\x30\x25\x75\xD0\xFB\x98\xD1\x16\xBC\x4B\x6D\xDE"\
                        "\xBC\xA3\xA5\xA7\x93\x9F\x02\x01\x01\x03\x3A\x00\x04"\
                        "\x3D\x6A\x7C\x2A\x6F\x20\x5F\x83\x9B\x04\x14\xEC\x58"\
                        "\xC6\xC7\x1B\x75\xF5\x15\xDE\xC3\xAE\x73\x3B\x5F\x47"\
                        "\x88\xDD\xC8\x15\xF0\x5B\xC1\xF6\x53\x8F\xD9\x69\x54"\
                        "\xE1\xF8\x40\xA2\xE2\x18\x99\x62\xCC\xAA\x14\x90\x08"\
                        "\x24\xC7\xDD\xB9\xA3\x81\xD1\x30\x81\xCE\x30\x0E\x06"\
                        "\x03\x55\x1D\x0F\x01\x01\xFF\x04\x04\x03\x02\x07\x80"\
                        "\x30\x1F\x06\x03\x55\x1D\x23\x04\x18\x30\x16\x80\x14"\
                        "\x60\x44\xF2\x45\xF2\xE0\x71\xD4\xD5\x64\xF4\xE5\x77"\
                        "\xD6\x36\x69\xDB\xEB\x18\x59\x30\x41\x06\x03\x55\x1D"\
                        "\x20\x04\x3A\x30\x38\x30\x36\x06\x09\x04\x00\x7F\x00"\
                        "\x07\x03\x01\x01\x01\x30\x29\x30\x27\x06\x08\x2B\x06"\
                        "\x01\x05\x05\x07\x02\x01\x16\x1B\x68\x74\x74\x70\x3A"\
                        "\x2F\x2F\x77\x77\x77\x2E\x62\x73\x69\x2E\x62\x75\x6E"\
                        "\x64\x2E\x64\x65\x2F\x63\x73\x63\x61\x30\x2B\x06\x09"\
                        "\x2A\x86\x48\x86\xF7\x0D\x01\x09\x15\x04\x1E\x04\x1C"\
                        "\x31\x2E\x32\x2E\x32\x37\x36\x2E\x30\x2E\x38\x30\x2E"\
                        "\x31\x2E\x31\x32\x2E\x30\x2E\x32\x30\x2E\x35\x2E\x31"\
                        "\x2E\x30\x30\x2B\x06\x03\x55\x1D\x10\x04\x24\x30\x22"\
                        "\x80\x0F\x32\x30\x31\x30\x31\x30\x30\x35\x31\x31\x30"\
                        "\x37\x35\x32\x5A\x81\x0F\x32\x30\x31\x31\x30\x32\x30"\
                        "\x35\x30\x39\x33\x39\x34\x37\x5A\x30\x0C\x06\x08\x2A"\
                        "\x86\x48\xCE\x3D\x04\x03\x02\x05\x00\x03\x48\x00\x30"\
                        "\x45\x02\x20\x13\xE9\xE1\x7A\x9E\xFE\x8B\xD7\xD7\x27"\
                        "\x62\x92\x30\x5B\xCC\xC3\x2B\x70\xC2\xB7\x60\x40\xF4"\
                        "\x88\x30\x66\x62\x26\xCD\x6A\x4B\xF4\x02\x21\x00\x87"\
                        "\xF4\x71\xE2\x44\x35\xB4\xC3\x4A\xF3\x57\x30\x94\xFB"\
                        "\x1F\x1C\x2A\x48\xB1\x3E\xE5\xED\x67\xF1\x72\x6D\xCF"\
                        "\x56\xE3\x84\xE3\x6F\x31\x82\x01\x09\x30\x82\x01\x05"\
                        "\x02\x01\x01\x30\x55\x30\x4F\x31\x0B\x30\x09\x06\x03"\
                        "\x55\x04\x06\x13\x02\x44\x45\x31\x0D\x30\x0B\x06\x03"\
                        "\x55\x04\x0A\x0C\x04\x62\x75\x6E\x64\x31\x0C\x30\x0A"\
                        "\x06\x03\x55\x04\x0B\x0C\x03\x62\x73\x69\x31\x0C\x30"\
                        "\x0A\x06\x03\x55\x04\x05\x13\x03\x30\x31\x33\x31\x15"\
                        "\x30\x13\x06\x03\x55\x04\x03\x0C\x0C\x63\x73\x63\x61"\
                        "\x2D\x67\x65\x72\x6D\x61\x6E\x79\x02\x02\x01\x5C\x30"\
                        "\x0D\x06\x09\x60\x86\x48\x01\x65\x03\x04\x02\x01\x05"\
                        "\x00\xA0\x4A\x30\x17\x06\x09\x2A\x86\x48\x86\xF7\x0D"\
                        "\x01\x09\x03\x31\x0A\x06\x08\x04\x00\x7F\x00\x07\x03"\
                        "\x02\x01\x30\x2F\x06\x09\x2A\x86\x48\x86\xF7\x0D\x01"\
                        "\x09\x04\x31\x22\x04\x20\xEF\x0F\xDA\x94\x2E\x5A\x0F"\
                        "\x6F\xC9\xC5\x46\xEE\x01\xF9\x10\x31\x43\x64\x30\xF7"\
                        "\x5E\x9D\x36\x54\xD3\x69\x30\x9E\x8B\xE7\x17\x48\x30"\
                        "\x0C\x06\x08\x2A\x86\x48\xCE\x3D\x04\x03\x02\x05\x00"\
                        "\x04\x40\x30\x3E\x02\x1D\x00\x89\x75\x92\x5B\xE1\x31"\
                        "\xB7\x7C\x95\x8C\x3E\xCB\x2A\x5C\x67\xFC\x5C\xE3\x1C"\
                        "\xBD\x01\x41\xE3\x4B\xC7\xF0\xA4\x47\x02\x1D\x00\xCC"\
                        "\x65\xE6\x2D\xDC\xF2\x93\x96\x4B\x22\xD7\xB5\x10\xD7"\
                        "\x81\x88\x07\xC8\x95\x96\xBD\x34\xD8\xF9\xBB\x4C\x05"\
                        "\x27"
        self.mf.append(TransparentStructureEF(parent=self.mf, fid=0x011d,
                       shortfid=0x1d, data=card_security))
        ef_dir = "\x61\x32\x4F\x0F\xE8\x28\xBD\x08\x0F\xA0\x00\x00\x01\x67"\
                 "\x45\x53\x49\x47\x4E\x50\x0F\x43\x49\x41\x20\x7A\x75\x20"\
                 "\x44\x46\x2E\x65\x53\x69\x67\x6E\x51\x00\x73\x0C\x4F\x0A"\
                 "\xA0\x00\x00\x01\x67\x45\x53\x49\x47\x4E\x61\x09\x4F\x07"\
                 "\xA0\x00\x00\x02\x47\x10\x01\x61\x0B\x4F\x09\xE8\x07\x04"\
                 "\x00\x7F\x00\x07\x03\x02\x61\x0C\x4F\x0A\xA0\x00\x00\x01"\
                 "\x67\x45\x53\x49\x47\x4E"
        self.mf.append(TransparentStructureEF(parent=self.mf, fid=0x2f00,
                       shortfid=30, data=ef_dir))

        eid = DF(parent=self.mf, fid=0xffff,
                 dfname='\xe8\x07\x04\x00\x7f\x00\x07\x03\x02')
        eid.extra_fci_data = "\xab\x75\x84\x01\xa4\xaf\x70\xa0\x10\xb4\x06"\
                             "\x95\x01\x20\x83\x01\x04\xb4\x06\x95\x01\x20"\
                             "\x83\x01\x06\xa0\x54\xa4\x14\x95\x01\x80\x7f"\
                             "\x4c\x0e\x06\x09\x04\x00\x7f\x00\x07\x03\x01"\
                             "\x02\x01\x53\x01\x00\xaf\x22\xa4\x18\x95\x01"\
                             "\x80\x7f\x4c\x12\x06\x09\x04\x00\x7f\x00\x07"\
                             "\x03\x01\x02\x02\x53\x05\x00\x00\x00\x00\x00"\
                             "\xa4\x06\x95\x01\x80\x83\x01\x47\xa4\x18\x95"\
                             "\x01\x80\x7f\x4c\x12\x06\x09\x04\x00\x7f\x00"\
                             "\x07\x03\x01\x02\x02\x53\x05\x00\x00\x00\x00"\
                             "\x10\x7a\x06\x8a\x01\x05\x9e\x01\x03"
        # FIXME access control for eid application

        DocumentType = self.datagroups["DocumentType"] if "DocumentType" in \
            self.datagroups else 'ID'
        IssuingState = self.datagroups["IssuingState"] if "IssuingState" in \
            self.datagroups else 'D'
        DateOfExpiry = self.datagroups["DateOfExpiry"] if "DateOfExpiry" in \
            self.datagroups else '20201031'
        GivenNames = self.datagroups["GivenNames"] if "GivenNames" in \
            self.datagroups else 'ERIKA'
        FamilyNames = self.datagroups["FamilyNames"] if "FamilyNames" in \
            self.datagroups else 'MUSTERMANN'
        ReligiousArtisticName = self.datagroups["ReligiousArtisticName"] if \
            "ReligiousArtisticName" in self.datagroups else ''
        AcademicTitle = self.datagroups["AcademicTitle"] if \
            "AcademicTitle" in self.datagroups else ''
        DateOfBirth = self.datagroups["DateOfBirth"] if "DateOfBirth" in \
            self.datagroups else '19640812'
        PlaceOfBirth = self.datagroups["PlaceOfBirth"] if "PlaceOfBirth" in \
            self.datagroups else 'BERLIN'
        Nationality = self.datagroups["Nationality"] if "Nationality" in \
            self.datagroups else 'DE'
        Sex = self.datagroups["Sex"] if "Sex" in self.datagroups else 'F'
        BirthName = self.datagroups["BirthName"] if "BirthName" in \
            self.datagroups else 'Mein Geburtsname'
        # PlaceOfResidence variable is only a helper to get a switch for
        # <NotOnChip>
        PlaceOfResidence = self.datagroups["PlaceOfResidence"] if \
            "PlaceOfResidence" in self.datagroups else ''
        Country = self.datagroups["Country"] if "Country" in self.datagroups \
            else 'D'
        City = self.datagroups["City"] if "City" in self.datagroups else \
            'KOLN'
        ZIP = self.datagroups["ZIP"] if "ZIP" in self.datagroups else '51147'
        Street = self.datagroups["Street"] if "Street" in \
            self.datagroups else 'HEIDESTRASSE 17'
        CommunityID = self.datagroups["CommunityID"] if "CommunityID" in \
            self.datagroups else '02760378900276'
        if (CommunityID.rstrip() != "<NotOnChip>"):
            # the plain CommunityID integer value has to be translated into
            # its binary representation. '0276...' will be '\x02\x76\...'
            CommunityID_Binary = binascii.unhexlify(CommunityID)
        # ResidencePermit1 and ResidencePermit2 are part of eAT only
        ResidencePermit1 = self.datagroups["ResidencePermit1"] if \
            "ResidencePermit1" in self.datagroups else \
            'ResidencePermit1 field up to 750 characters'
        ResidencePermit2 = self.datagroups["ResidencePermit2"] if \
            "ResidencePermit2" in self.datagroups else \
            'ResidencePermit1 field up to 250 characters'

        # Currently, those data groups are for further usage:
        dg12_param = self.datagroups["dg12"] if "dg12" in \
            self.datagroups else ''
        dg14_param = self.datagroups["dg14"] if "dg14" in \
            self.datagroups else ''
        dg15_param = self.datagroups["dg15"] if "dg15" in \
            self.datagroups else ''
        dg16_param = self.datagroups["dg16"] if "dg16" in \
            self.datagroups else ''
        dg21_param = self.datagroups["dg21"] if "dg21" in \
            self.datagroups else ''

        # "Attribute not on Chip" makes sence only for ReligiousArtisticName,
        # Nationality, BirthName, ResidencePermit1 and ResidencePermit2, refer
        # to BSI TR-03127
        if (DocumentType.rstrip() != "<NotOnChip>"):
            dg1 = pack([(0x61, 0, [(0x13, 0, DocumentType)])], True)
        else:
            dg1 = None
        if (IssuingState.rstrip() != "<NotOnChip>"):
            dg2 = pack([(0x62, 0, [(0x13, 0, IssuingState)])], True)
        else:
            dg2 = None
        if (DateOfExpiry.rstrip() != "<NotOnChip>"):
            dg3 = pack([(0x63, 0, [(0x12, 0, DateOfExpiry)])], True)
        else:
            dg3 = None
        if (GivenNames.rstrip() != "<NotOnChip>"):
            dg4 = pack([(0x64, 0, [(0x0C, 0, GivenNames)])], True)
        else:
            dg4 = None
        if (FamilyNames.rstrip() != "<NotOnChip>"):
            dg5 = pack([(0x65, 0, [(0x0C, 0, FamilyNames)])], True)
        else:
            dg5 = None
        if (ReligiousArtisticName.rstrip() != "<NotOnChip>"):
            dg6 = pack([(0x66, 0, [(0x0C, 0, ReligiousArtisticName)])], True)
        else:
            dg6 = None
        if (AcademicTitle.rstrip() != "<NotOnChip>"):
            dg7 = pack([(0x67, 0, [(0x0C, 0, AcademicTitle)])], True)
        else:
            dg7 = None
        if (DateOfBirth.rstrip() != "<NotOnChip>"):
            dg8 = pack([(0x68, 0, [(0x12, 0, DateOfBirth)])], True)
        else:
            dg8 = None
        if (PlaceOfBirth.rstrip() != "<NotOnChip>"):
            dg9 = pack([(0x69, 0, [(0xA1, 0, [(0x0C, 0, PlaceOfBirth)])])],
                       True)
        else:
            dg9 = None
        if (Nationality.rstrip() != "<NotOnChip>"):
            dg10 = pack([(0x6A, 0, [(0x13, 0, Nationality)])], True)
        else:
            dg10 = None
        if (Sex.rstrip() != "<NotOnChip>"):
            dg11 = pack([(0x6B, 0, [(0x13, 0, Sex)])], True)
        else:
            dg11 = None
        if (dg12_param.rstrip() != "<NotOnChip>"):
            dg12 = dg12_param
        else:
            dg12 = None
        if (BirthName.rstrip() != "<NotOnChip>"):
            dg13 = pack([(0x6D, 0, [(0x0C, 0, BirthName)])], True)
        else:
            dg13 = None
        if (dg14_param.rstrip() != "<NotOnChip>"):
            dg14 = dg14_param
        else:
            dg14 = None
        if (dg15_param.rstrip() != "<NotOnChip>"):
            dg15 = dg15_param
        else:
            dg15 = None
        if (dg16_param.rstrip() != "<NotOnChip>"):
            dg16 = dg16_param
        else:
            dg16 = None
        if (PlaceOfResidence.rstrip() != "<NotOnChip>"):
            dg17 = pack([(0x71, 0, [(0x30, 0, [
                    (0xAA, 0, [(0x0C, 0, Street)]),
                    (0xAB, 0, [(0x0C, 0, City)]),
                    (0xAD, 0, [(0x13, 0, Country)]),
                    (0xAE, 0, [(0x13, 0, ZIP)])
                    ])])], True)
        else:
            dg17 = None
        if (CommunityID.rstrip() != "<NotOnChip>"):
            dg18 = pack([(0x72, 0, [(0x04, 0, CommunityID_Binary)])], True)
        else:
            dg18 = None
        if (ResidencePermit1.rstrip() != "<NotOnChip>"):
            dg19 = pack([(0x73, 0, [(0xA1, 0,
                                    [(0x0C, 0, ResidencePermit1)])])], True)
        else:
            dg19 = None
        if (ResidencePermit1.rstrip() != "<NotOnChip>"):
            dg20 = pack([(0x74, 0, [(0xA1, 0,
                                    [(0x0C, 0, ResidencePermit2)])])], True)
        else:
            dg20 = None
        if (dg21_param.rstrip() != "<NotOnChip>"):
            dg21 = dg21_param
        else:
            dg21 = None

        # If eid.append is not done for a DG, it results into required
        # SwError() with FileNotFound "6A82" APDU return code
        if dg1:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0101,
                       shortfid=0x01, data=dg1))
        if dg2:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0102,
                       shortfid=0x02, data=dg2))
        if dg3:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0103,
                       shortfid=0x03, data=dg3))
        if dg4:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0104,
                       shortfid=0x04, data=dg4))
        if dg5:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0105,
                       shortfid=0x05, data=dg5))
        if dg6:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0106,
                       shortfid=0x06, data=dg6))
        if dg7:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0107,
                       shortfid=0x07, data=dg7))
        if dg8:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0108,
                       shortfid=0x08, data=dg8))
        if dg9:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0109,
                       shortfid=0x09, data=dg9))
        if dg10:
            eid.append(TransparentStructureEF(parent=eid, fid=0x010a,
                       shortfid=0x0a, data=dg10))
        if dg11:
            eid.append(TransparentStructureEF(parent=eid, fid=0x010b,
                       shortfid=0x0b, data=dg11))
        if dg12:
            eid.append(TransparentStructureEF(parent=eid, fid=0x010c,
                       shortfid=0x0c, data=dg12))
        if dg13:
            eid.append(TransparentStructureEF(parent=eid, fid=0x010d,
                       shortfid=0x0d, data=dg13))
        if dg14:
            eid.append(TransparentStructureEF(parent=eid, fid=0x010e,
                       shortfid=0x0e, data=dg14))
        if dg15:
            eid.append(TransparentStructureEF(parent=eid, fid=0x010f,
                       shortfid=0x0f, data=dg15))
        if dg16:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0110,
                       shortfid=0x10, data=dg16))
        if dg17:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0111,
                       shortfid=0x11, data=dg17))
        if dg18:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0112,
                       shortfid=0x12, data=dg18))
        if dg19:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0113,
                       shortfid=0x13, data=dg19))
        if dg20:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0114,
                       shortfid=0x14, data=dg20))
        if dg21:
            eid.append(TransparentStructureEF(parent=eid, fid=0x0115,
                       shortfid=0x15, data=dg21))
        self.mf.append(eid)

        # DF.CIA
        cia = DF(parent=self.mf, fid=0xfffe,
                 dfname='\xE8\x28\xBD\x08\x0F\xA0\x00\x00\x01\x67\x45\x53\x49'
                        '\x47\x4E')
        # EF.OD / EF.ODF
        cia.append(TransparentStructureEF(parent=cia, fid=0x5031,
                                          shortfid=0x11,
                                          data='\xa0\x060\x04\x04\x02D\x00\xa4'
                                          '\x060\x04\x04\x02D\x04\xa8\x060\x04'
                                          '\x04\x02D\x08'))
        # EF.CIAInfo / EF.TokenInfo
        cia.append(TransparentStructureEF(parent=cia, fid=0x5032,
                                          shortfid=0x12,
                                          data='06\x02\x01\x01\x80\x11eSign '
                                          'Application\x03\x02\x06\xc0\xa2'
                                          '\x1a0\x18\x02\x01\x01\x02\x02\x10A'
                                          '\x05\x00\x03\x02\x06@\x06\t\x04\x00'
                                          '\x7f\x00\x07\x01\x01\x04\x01'))
        # EF.PrKD / EF.PrKDF
        cia.append(TransparentStructureEF(parent=cia, fid=0x4400,
                                          data='\xa080\x17\x0c\x0bPrK.ICC.QES'
                                          '\x03\x02\x07\x80\x04\x01\x01\x02'
                                          '\x01\x010\x15\x04\x01F\x03\x03\x06'
                                          '\x00@\x03\x02\x03\xb8\x02\x02\x00'
                                          '\x84\xa1\x03\x02\x01\x01\xa1\x060'
                                          '\x040\x02\x04\x00'))
        # EF.PuKD / EF.PuKDF
        cia.append(TransparentStructureEF(parent=cia, fid=0x4404,
                                          data='050!\x0c\x1fZertifikat des ZDA'
                                          ' f\xc3\xbcr die QES0\x06\x04\x01E'
                                          '\x01\x01\xff\xa1\x080\x060\x04\x04'
                                          '\x02\xc0\x000:0&\x0c$Zertifikat des'
                                          ' Inhabers f\xc3\xbcr die QES0\x06'
                                          '\x04\x01F\x01\x01\x00\xa1\x080\x060'
                                          '\x04\x04\x02\xc0\x01'))
        # EF.AOD / EF.AODF
        cia.append(TransparentStructureEF(parent=cia, fid=0x4408,
                                          data='0/0\x0f\x0c\teSign-PIN\x03\x02'
                                          '\x06@0\x03\x04\x01\x01\xa1\x170\x15'
                                          '\x03\x03\x02H\x1c\n\x01\x01\x02\x01'
                                          '\x06\x02\x01\x00\x80\x01\x810\x02'
                                          '\x04\x00'))
        self.mf.append(cia)

        # DF.eSign
        esign = DF(parent=self.mf, fid=0xfffd,
                   dfname='\xA0\x00\x00\x01\x67\x45\x53\x49\x47\x4E')

        # ZDA certificate
        esign.append(TransparentStructureEF(parent=esign, fid=0xC000, data=''))
        # User's certificate
        esign.append(TransparentStructureEF(parent=esign, fid=0xC001, data=''))
        self.mf.append(esign)

        self.sam = nPA_SAM(eid_pin="111111", can="222222",
                           mrz="IDD<<T220001293<<<<<<<<<<<<<<<6408125<2010315D"
                           "<<<<<<<<<<<<<<MUSTERMANN<<ERIKA<<<<<<<<<<<<<",
                           puk="3333333333", qes_pin="444444", mf=self.mf)
        # FIXME: add CVCA for inspection systems and signature terminals. Here
        #        we only add the eID CVCA.
        self.sam.current_SE.cvca = "\x7f\x21\x82\x01\xb6\x7f\x4e\x82\x01\x6e"\
                                   "\x5f\x29\x01\x00\x42\x0e\x44\x45\x43\x56"\
                                   "\x43\x41\x65\x49\x44\x30\x30\x31\x30\x32"\
                                   "\x7f\x49\x82\x01\x1d\x06\x0a\x04\x00\x7f"\
                                   "\x00\x07\x02\x02\x02\x02\x03\x81\x20\xa9"\
                                   "\xfb\x57\xdb\xa1\xee\xa9\xbc\x3e\x66\x0a"\
                                   "\x90\x9d\x83\x8d\x72\x6e\x3b\xf6\x23\xd5"\
                                   "\x26\x20\x28\x20\x13\x48\x1d\x1f\x6e\x53"\
                                   "\x77\x82\x20\x7d\x5a\x09\x75\xfc\x2c\x30"\
                                   "\x57\xee\xf6\x75\x30\x41\x7a\xff\xe7\xfb"\
                                   "\x80\x55\xc1\x26\xdc\x5c\x6c\xe9\x4a\x4b"\
                                   "\x44\xf3\x30\xb5\xd9\x83\x20\x26\xdc\x5c"\
                                   "\x6c\xe9\x4a\x4b\x44\xf3\x30\xb5\xd9\xbb"\
                                   "\xd7\x7c\xbf\x95\x84\x16\x29\x5c\xf7\xe1"\
                                   "\xce\x6b\xcc\xdc\x18\xff\x8c\x07\xb6\x84"\
                                   "\x41\x04\x8b\xd2\xae\xb9\xcb\x7e\x57\xcb"\
                                   "\x2c\x4b\x48\x2f\xfc\x81\xb7\xaf\xb9\xde"\
                                   "\x27\xe1\xe3\xbd\x23\xc2\x3a\x44\x53\xbd"\
                                   "\x9a\xce\x32\x62\x54\x7e\xf8\x35\xc3\xda"\
                                   "\xc4\xfd\x97\xf8\x46\x1a\x14\x61\x1d\xc9"\
                                   "\xc2\x77\x45\x13\x2d\xed\x8e\x54\x5c\x1d"\
                                   "\x54\xc7\x2f\x04\x69\x97\x85\x20\xa9\xfb"\
                                   "\x57\xdb\xa1\xee\xa9\xbc\x3e\x66\x0a\x90"\
                                   "\x9d\x83\x8d\x71\x8c\x39\x7a\xa3\xb5\x61"\
                                   "\xa6\xf7\x90\x1e\x0e\x82\x97\x48\x56\xa7"\
                                   "\x86\x41\x04\x33\x47\xec\xf9\x6f\xfb\x4b"\
                                   "\xd9\xb8\x55\x4e\xfb\xcc\xfc\x7d\x0b\x24"\
                                   "\x2f\x10\x71\xe2\x9b\x4c\x9c\x62\x2c\x79"\
                                   "\xe3\x39\xd8\x40\xaf\x67\xbe\xb9\xb9\x12"\
                                   "\x69\x22\x65\xd9\xc1\x6c\x62\x57\x3f\x45"\
                                   "\x79\xff\xd4\xde\x2d\xe9\x2b\xab\x40\x9d"\
                                   "\xd5\xc5\xd4\x82\x44\xa9\xf7\x87\x01\x01"\
                                   "\x5f\x20\x0e\x44\x45\x43\x56\x43\x41\x65"\
                                   "\x49\x44\x30\x30\x31\x30\x32\x7f\x4c\x12"\
                                   "\x06\x09\x04\x00\x7f\x00\x07\x03\x01\x02"\
                                   "\x02\x53\x05\xfe\x0f\x01\xff\xff\x5f\x25"\
                                   "\x06\x01\x00\x01\x00\x01\x08\x5f\x24\x06"\
                                   "\x01\x03\x01\x00\x01\x08\x5f\x37\x40\x50"\
                                   "\x67\x14\x5c\x68\xca\xe9\x52\x0f\x5b\xb3"\
                                   "\x48\x17\xf1\xca\x9c\x43\x59\x3d\xb5\x64"\
                                   "\x06\xc6\xa3\xb0\x06\xcb\xf3\xf3\x14\xe7"\
                                   "\x34\x9a\xcf\x0c\xc6\xbf\xeb\xcb\xde\xfd"\
                                   "\x10\xb4\xdc\xf0\xf2\x31\xda\x56\x97\x7d"\
                                   "\x88\xf9\xf9\x01\x82\xd1\x99\x07\x6a\x56"\
                                   "\x50\x64\x51"

    def __generate_cryptoflex(self):
        """Generate the Filesystem and SAM of a cryptoflex card"""
        from virtualsmartcard.cards.cryptoflex import CryptoflexMF
        from virtualsmartcard.cards.cryptoflex import CryptoflexSAM
        self.mf = CryptoflexMF()
        self.mf.append(TransparentStructureEF(parent=self.mf, fid=0x0002,
                       filedescriptor=0x01,
                       data="\x00\x00\x00\x01\x00\x01\x00\x00"))  # EF.ICCSN
        self.sam = CryptoflexSAM(self.mf)

    def __generate_belpic(self):
        from virtualsmartcard.cards.belpic import BelpicMF
        self.__generate_iso_card()
        self.mf = BelpicMF('belpic.xml')
        self.sam.set_MF(self.mf)

    def generateCard(self):
        """Generate a new card"""
        if self.type == 'iso7816':
            self.__generate_iso_card()
        elif self.type == 'ePass':
            self.__generate_ePass()
        elif self.type == 'cryptoflex':
            self.__generate_cryptoflex()
        elif self.type == 'nPA':
            self.__generate_nPA()
        elif self.type == 'belpic':
            self.__generate_belpic()
        else:
            return (None, None)

    def getCard(self):
        """Get the MF and SAM from the current card"""
        if self.sam is None or self.mf is None:
            self.generateCard()
        return self.mf, self.sam

    def setCard(self, mf=None, sam=None):
        """Set the MF and SAM of the current card"""
        if mf is not None:
            self.mf = mf
        if sam is not None:
            self.sam = sam

    def readDatagroups(self, datasetfile):
        """Read Datagroups from file"""
        with open(datasetfile, 'r') as f:
            for line in f:
                if (not line.startswith("#")) and (not len(line.strip()) == 0):
                    # spaces after equal sign are allowed to get strings with
                    # leading spaces
                    line = line.replace(" =", "=")
                    splitLine = line.split("=")
                    # we don't want to have the newline char from dataset file
                    # as part of the value
                    self.datagroups[splitLine[0]] = splitLine[1].rstrip("\n\r")
                    logging.info("Dataset value for " + splitLine[0].rstrip() +
                                 ": '" + splitLine[1].rstrip("\n\r") + "'")

if __name__ == "__main__":
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("-t", "--type", action="store", type="choice",
                      default='iso7816',
                      choices=['iso7816', 'cryptoflex', 'ePass', 'nPA'],
                      help="Type of Smartcard [default: %default]")
    (options, args) = parser.parse_args()

    generator = CardGenerator(options.type)
    generator.generateCard()
