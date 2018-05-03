#
# Copyright (C) 2011-2015 Frank Morgner
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

from virtualsmartcard.VirtualSmartcard import SmartcardOS

import logging


class HandlerTestOS(SmartcardOS):
    """
    This class implements the commands used for the PC/SC-lite  smart  card
    reader driver tester. See http://pcsclite.alioth.debian.org/pcsclite.html
    and handler_test(1).
    """
    def __init__(self):
        lastCommandOffcut = ''

    def getATR(self):
        return '\x3B\xD6\x18\x00\x80\xB1\x80\x6D\x1F\x03\x80\x51\x00\x61' + \
               '\x10\x30\x9E'

    def __output_from_le(self, msg):
        le = (ord(msg[2]) << 8) + ord(msg[3])
        return ''.join([bytes(num & 0xff) for num in xrange(le)])

    def execute(self, msg):
        ok = '\x90\x00'
        error = '\x6d\x00'
        if (msg == '\x00\xA4\x04\x00\x06\xA0\x00\x00\x00\x18\x50' or
                msg == '\x00\xA4\x04\x00\x06\xA0\x00\x00\x00\x18\xFF'):
            logging.info('Select applet')
            return ok
        elif msg.startswith('\x80\x38\x00'):
            logging.info('Time Request')
            return ok
        elif msg == '\x80\x30\x00\x00':
            logging.info('Case 1, APDU')
            return ok
        elif msg == '\x80\x30\x00\x00\x00':
            logging.info('Case 1, TPDU')
            return ok
        elif msg.startswith('\x80\x32\x00\x00'):
            logging.info('Case 3')
            return ok
        elif msg.startswith('\x80\x34'):
            logging.info('Case 2')
            return self.__output_from_le(msg) + ok
        elif msg.startswith('\x80\x36'):
            if len(msg) == 5+ord(msg[4]):
                logging.info('Case 4, TPDU')
                self.lastCommandOffcut = self.__output_from_le(msg)
                if len(self.lastCommandOffcut) > 0xFF:
                    return '\x61\x00'
                return '' + bytes(len(self.lastCommandOffcut) & 0xFF)
            elif len(msg) == 6+ord(msg[4]):
                logging.info('Case 4, APDU')
                return self.__output_from_le(msg) + ok
            else:
                return error
        elif msg.startswith('\x80\xC0\x00\x00'):
            logging.info('Get response')
            out = self.lastCommandOffcut[:ord(msg[4])]
            self.lastCommandOffcut = self.lastCommandOffcut[ord(msg[4]):]
            return out + ok
        else:
            return error
