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
from virtualsmartcard.ConstantDefinitions import MAX_SHORT_LE
from virtualsmartcard.SmartcardFilesystem import MF

class BelpicOS(Iso7816OS):
    def __init__(self, mf, sam, ins2handler=None, maxle=MAX_SHORT_LE):
        Iso7816OS.__init__(self, mf, sam, ins2handler, maxle)
        self.atr = '\x3B\x98\x13\x40\x0A\xA5\x03\x01\x01\x01\xAD\x13\x11'


    @staticmethod
    def create(p1, p2, data):
        raise SwError(SW["ERR_INSNOTSUPPORTED"])
