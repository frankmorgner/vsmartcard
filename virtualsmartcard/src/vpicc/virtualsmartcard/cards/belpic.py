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
from virtualsmartcard.ConstantDefinitions import MAX_SHORT_LE, FDB, LCB
from virtualsmartcard.SmartcardFilesystem import MF, DF, TransparentStructureEF

import xml.etree.ElementTree as ET

class BelpicOS(Iso7816OS):
    def __init__(self, mf, sam, ins2handler=None, maxle=MAX_SHORT_LE):
        Iso7816OS.__init__(self, mf, sam, ins2handler, maxle)
        self.atr = '\x3B\x98\x13\x40\x0A\xA5\x03\x01\x01\x01\xAD\x13\x11'

class BelpicMF(MF):
    def __init__(self, datafile, filedescriptor=FDB["NOTSHAREABLEFILE" ] | FDB["DF"],
                 lifecycle=LCB["ACTIVATED"],simpletlv_data = None, bertlv_data = None):
        MF.__init__(self, filedescriptor, lifecycle, simpletlv_data, bertlv_data, dfname = "A00000003029057000AD13100101FF".decode('hex'))
        tree = ET.parse(datafile)
        root = tree.getroot()
        ns = {'f': 'urn:be:fedict:eid:dev:virtualcard:1.0'}
        DF00 = DF(self, 0xDF00)
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

    @staticmethod
    def create(p1, p2, data):
        raise SwError(SW["ERR_INSNOTSUPPORTED"])
