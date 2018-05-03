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

import unittest
from virtualsmartcard.CryptoUtils import *


class TestCryptoUtils(unittest.TestCase):

    def setUp(self):
        self.teststring = b"DEADBEEFistatsyksdvhwohfwoehcowc8hw8rogfq8whv75tsg"\
                          b"ohsav8wress"

    def test_padding(self):
        padded = append_padding(16, self.teststring)
        unpadded = strip_padding(16, padded)
        self.assertEqual(unpadded, self.teststring)

if __name__ == "__main__":
    unittest.main()
