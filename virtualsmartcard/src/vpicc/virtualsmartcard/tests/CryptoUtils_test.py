import unittest
from virtualsmartcard.CryptoUtils import *

class TestCryptoUtils(unittest.TestCase):

    def setUp(self):
        self.teststring = "DEADBEEFistatsyksdvhwohfwoehcowc8hw8rogfq8whv75tsgohsav8wress"
        self.testpass = "SomeRandomPassphrase"

    def test_padding(self):
        padded = append_padding(16, self.teststring)
        unpadded = strip_padding(16, padded)
        self.assertEqual(unpadded, self.teststring)

    def test_protect_string(self):
        protectedString = protect_string(self.teststring, self.testpass)
        unprotectedString = read_protected_string(protectedString, self.testpass)
        self.assertEqual(self.teststring, unprotectedString)


if __name__ == "__main__":
    unittest.main()    
    #test_pbkdf2()
