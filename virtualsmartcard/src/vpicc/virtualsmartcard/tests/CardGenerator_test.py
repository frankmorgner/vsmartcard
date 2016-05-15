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

import os
import tempfile
import unittest

from virtualsmartcard.CardGenerator import CardGenerator


class ISO7816GeneratorTest(unittest.TestCase):

    card_type = 'iso7816'

    def setUp(self):
        self.card_generator = CardGenerator(self.card_type)

    def test_card_creation(self):
        self.card_generator.generateCard()
        mf, sam = self.card_generator.getCard()
        self.assertIsNotNone(mf)
        self.assertIsNotNone(sam)

    def test_get_and_set_card(self):
        self.card_generator.generateCard()
        mf, sam = self.card_generator.getCard()
        local_generator = CardGenerator(self.card_type)
        local_generator.setCard(mf, sam)


class TestNPACardGenerator(ISO7816GeneratorTest):

    card_type = 'nPA'

    def setUp(self):
        self.card_generator = CardGenerator(self.card_type)
        self.test_readDatagroups_file = "/../../../../npa-example-data/"\
                                        "Example_Dataset_Mueller_Gertrud.txt"

    def test_readDatagroups(self):
        path = os.path.dirname(__file__)
        datagroupsFile = path + self.test_readDatagroups_file
        self.card_generator.readDatagroups(datagroupsFile)
        mf, sam = self.card_generator.getCard()
        self.assertIsNotNone(mf)
        self.assertIsNotNone(sam)


class CryptoflexGeneratorTest(ISO7816GeneratorTest):

    card_type = 'cryptoflex'

# Not tested because an ePass card currently cannot be generated without user
# interaction.
#
# class ePassGeneratorTest(ISO7816GeneratorTest):
#
#   card_type = 'ePass'

if __name__ == '__main__':
    unittest.main()
