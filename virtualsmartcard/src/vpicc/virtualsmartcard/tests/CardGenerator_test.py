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
import tempfile
import os
from virtualsmartcard.CardGenerator import CardGenerator

class TestNPACardGenerator(unittest.TestCase):

    def setUp(self):
        self.filename = tempfile.mktemp()
        self.nPA_generator = CardGenerator('nPA')
        self.nPA_generator.password = "TestPassword"

    def tearDown(self):
        os.unlink(self.filename)

    def test_nPA_creation(self):
        self.nPA_generator.generateCard()
        self.nPA_generator.saveCard(self.filename)
        mf, sam = self.nPA_generator.getCard()
        self.assertIsNotNone(mf)
        self.assertIsNotNone(sam)

    def test_load_nPA_from_file_nPA_from_file(self):
        self.nPA_generator.generateCard()
        self.nPA_generator.saveCard(self.filename)
        local_generator= CardGenerator('nPA')
        local_generator.password = self.nPA_generator.password
        local_generator.loadCard(self.filename)
        mf, sam = local_generator.getCard()
        self.assertIsNotNone(mf)
        self.assertIsNotNone(sam)

if __name__ == '__main__':
    unittest.main()
