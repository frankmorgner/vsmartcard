#
# Copyright (C) 2017 Wouter Verhelst, Federal Public Service BOSA, DG DT
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

from virtualsmartcard.VirtualSmartcard import Iso7816OS
from virtualsmartcard.ConstantDefinitions import MAX_SHORT_LE, FDB, LCB, REF
from virtualsmartcard.SmartcardFilesystem import MF, DF, TransparentStructureEF
from virtualsmartcard.SWutils import SW, SwError
from virtualsmartcard.utils import C_APDU, R_APDU

import logging

import xml.etree.ElementTree as ET

class BelpicOS(Iso7816OS):
    def __init__(self, mf, sam, ins2handler=None, maxle=MAX_SHORT_LE):
        Iso7816OS.__init__(self, mf, sam, ins2handler, maxle)
        self.ins2handler = {
            0xc0: self.getResponse,
            0xa4: self.mf.selectFile,
            0xb0: self.mf.readBinaryPlain,
            0x20: self.SAM.verify,
            0x24: self.SAM.change_reference_data,
            0x22: self.SAM.manage_security_environment,
            0x2a: self.SAM.perform_security_operation,
            0xe4: self.getCardData,
            0xe6: self.logOff
        }
        self.atr = '\x3B\x98\x13\x40\x0A\xA5\x03\x01\x01\x01\xAD\x13\x11'

    # TODO: don't hardcode the value below, so that we can also emulate the v1.1 applet
    def getCardData(self, p1, p2, data):
        return SW["NORMAL"], "534C494E0123456789ABCDEF01234567F3360125011700030021010f".decode("hex")

    # TODO: actually log off
    def logOff(self, p1, p2, data):
        return SW["NORMAL"]

    def execute(self, msg):
        c = C_APDU(msg)
        if (c.ins == 0xa4 and c.p1 == 0x02):
            # The belpic applet is a bit loose with interpretation of
            # the ISO 7816 standard on the A4 command:
            # - The MF can be selected by name from anywhere with P1 ==
            #   0x02, rather than 0x00 as ISO 7816 requires
            if (c.data == '3F00'.decode('hex')):
                logging.info("Original APDU:\n%s\nRewritten to:\n", str(c))
                c.p1 = 0
                msg = c.render()
            # - Child DFs can be selected with P1 == 0x02, rather than
            #   0x01 as ISO 7816 requires
            if (c.data == 'DF00'.decode('hex') or c.data == 'DF01'.decode('hex')):
                logging.info("Original APDU:\n%s\nRewritten to:\n", str(c))
                c.p1 = 1
                msg = c.render()
        return Iso7816OS.execute(self, msg)

    def formatResult(self, seekable, le, data, sw, sm):
        r = R_APDU(Iso7816OS.formatResult(self, seekable, le, data, sw, sm))
        # The Belpic applet provides a bogus file length of 65536 for
        # every file, and does not return an error or warning when the
        # actual file length is shorter that the file as found; so
        # filter out the EOFBEFORENEREAD warning
        if (r.sw1 == 0x62 and r.sw2 == 0x82):
            logging.info("Filtering out warning")
            r.sw = "9000".decode("hex")
        return r.render()

class BelpicMF(MF):
    def __init__(self, datafile, filedescriptor=FDB["NOTSHAREABLEFILE" ] | FDB["DF"],
                 lifecycle=LCB["ACTIVATED"],simpletlv_data = None, bertlv_data = None):
        MF.__init__(self, filedescriptor, lifecycle, simpletlv_data, bertlv_data, dfname = "A00000003029057000AD13100101FF".decode('hex'))
        tree = ET.parse(datafile)
        root = tree.getroot()
        ns = {'f': 'urn:be:fedict:eid:dev:virtualcard:1.0'}
        DF00 = DF(self, 0xDF00, dfname="A000000177504B43532D3135".decode('hex'))
        self.append(DF00)
        DF01 = DF(self, 0xDF01)
        self.append(DF01)
        parent = self
        fid = 0
        for f in root.findall('f:file', ns):
            id = f.find('f:id', ns).text
            content = f.find('f:content', ns).text
            if content is not None:
                if(len(id) == 12):
                    fid = int(id[8:], 16)
                    if (id[4:8] == 'DF00'):
                        parent = DF00
                    if (id[4:8] == 'DF01'):
                        parent = DF01
                else:
                    fid = int(id[4:], 16)
                parent.append(TransparentStructureEF(parent, fid, data = content.decode('hex')))

    def select(self, attribute, value, reference=REF["IDENTIFIER_FIRST"], index_current=0):
        if (hasattr(self, attribute) and
                ((getattr(self, attribute) == value) or
                    (attribute == 'dfname' and
                        getattr(self, attribute).startswith(value)))):
            return self
        return DF.select(self, attribute, value, reference, index_current)
