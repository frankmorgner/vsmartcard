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
from virtualsmartcard.utils import C_APDU, R_APDU, hexdump


class TestUtils(unittest.TestCase):

    def test_CAPDU(self):
        a = C_APDU(1, 2, 3, 4)  # case 1
        b = C_APDU(1, 2, 3, 4, 5)  # case 2
        c = C_APDU((1, 2, 3), cla=0x23, data=b"hallo")  # case 3
        d = C_APDU(1, 2, 3, 4, 2, 4, 6, 0)  # case 4

        print()
        print(a)
        print(b)
        print(c)
        print(d)
        print()
        print(repr(a))
        print(repr(b))
        print(repr(c))
        print(repr(d))

        print()
        for i in a, b, c, d:
            print(hexdump(i.render()))

        # case 2 extended length
        print()
        g = C_APDU(0x00, 0xb0, 0x9c, 0x00, 0x00, 0x00, 0x00)

        print()
        print(g)
        print(repr(g))
        print(hexdump(g.render()))

        h = C_APDU(b'\x0c\x2a\x00\xbe\x00\x01\x5f\x87\x82\x01\x51\x01\xf0\xa2'
                   b'\x21\xa1\x36\x27\xb1\x30\x31\x3e\xd0\x97\x09\xb5\xde\x73'
                   b'\x5e\x29\x90\xce\xf1\x3d\x8a\xfd\xe7\x92\xe5\xa4\x70\xb9'
                   b'\x5d\x31\xe2\x34\xe7\xe2\x06\x13\x17\x7a\x3e\xca\x06\x39'
                   b'\x24\x2e\x75\x8c\x29\x6d\xd8\xa3\x1b\x1a\x68\x58\xd0\x1a'
                   b'\x98\xd4\xd8\x19\x50\xe9\x1b\x3c\xd1\xfd\x10\x53\x5b\xf2'
                   b'\x3b\xff\x4a\xf6\x05\xd0\x72\xad\xae\xaa\x93\x1a\x0a\x90'
                   b'\xc8\xa1\xb1\xf1\x0a\xba\x5b\xd2\x23\x38\xf8\x9a\x38\x9e'
                   b'\xa2\x04\x8b\xcb\x8b\x8b\xc0\x80\xd9\x2a\x04\x47\x26\x83'
                   b'\xda\xfe\x57\x68\x6b\x00\xb9\xa2\xea\x96\xf2\x07\x7f\xc5'
                   b'\x9c\xee\xbe\xf3\x81\xbf\x24\x19\x1e\x49\x1e\x9a\x85\x8f'
                   b'\x34\xcb\x1a\x23\xae\x6d\x7f\xa4\xb6\x7b\x60\x5d\x56\x79'
                   b'\x1c\xec\x18\xcc\x09\xdb\xb2\xbb\xf4\x31\xee\x08\x54\x26'
                   b'\xd5\xde\x99\xfa\x43\xa2\x49\x8e\x60\xc0\xaa\x4f\xfd\xf7'
                   b'\xe5\xc8\x89\x43\x5e\x11\xa2\x28\xc4\x92\x11\xda\xba\xe4'
                   b'\x91\xec\x04\xc9\x2c\xbd\x91\x6a\x5e\x7e\xb9\x85\xa2\xfa'
                   b'\x07\xc9\x47\x24\xa4\x3b\x63\xef\x75\x65\xef\xaf\xac\x22'
                   b'\x75\x99\x8b\x19\xde\x95\x76\xc9\xc8\xbc\x30\x23\x48\x07'
                   b'\x28\x19\x1e\x49\x9e\xcb\x99\xc3\x48\xdd\x1d\x0f\x44\x62'
                   b'\x64\x2a\x19\x7b\xeb\xee\xdf\xa1\xa6\xae\x87\x6d\x93\x36'
                   b'\x2d\x35\x8f\xd9\x61\x73\xef\x2d\x39\xb5\xc5\xe2\x75\x4b'
                   b'\x63\x0b\x41\x94\x8c\xbb\x55\xf6\x98\x5f\x9c\x07\xca\xe3'
                   b'\x15\xe4\xe6\x93\xd0\xa3\x9b\x22\xfa\x62\x18\xc5\x63\xfa'
                   b'\x2d\x57\xbb\x29\x2d\x57\x10\xd3\x0c\x05\x80\x15\x27\x4b'
                   b'\xc0\x84\x23\x62\x22\x6b\xae\x39\xa2\x8f\x55\xac\x8e\x08'
                   b'\x34\x46\xcc\x83\xf9\x9d\x2a\x75\x00\x00')
        print()
        print(h)
        print(repr(h))

    def test_RAPDU(self):
        e = R_APDU(0x90, 0)
        f = R_APDU(b"foo\x67\x00")

        print()
        print(e)
        print(f)
        print()
        print(repr(e))
        print(repr(f))

        print()
        for i in e, f:
            print(hexdump(i.render()))
