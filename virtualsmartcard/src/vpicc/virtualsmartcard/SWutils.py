#
# Copyright (C) 2011 Frank Morgner
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
# Meaning of the interindustry values of SW1-SW2
SW = {
        "NORMAL": 0x9000,
        "NORMAL_REST": 0x6100,
        "WARN_NOINFO62": 0x6200,
        "WARN_DATACORRUPTED": 0x6281,
        "WARN_EOFBEFORENEREAD": 0x6282,
        "WARN_DEACTIVATED": 0x6283,
        "WARN_FCIFORMATTING": 0x6284,
        "WARN_TERMINATIONSTATE": 0x6285,
        "WARN_NOINPUTSENSOR": 0x6286,
        "WARN_NOINFO63": 0x6300,
        "WARN_FILEFILLED": 0x6381,
        "ERR_EXECUTION": 0x6400,
        "ERR_RESPONSEREQUIRED": 0x6401,
        "ERR_NOINFO65": 0x6500,
        "ERR_MEMFAILURE": 0x6581,
        "ERR_WRONGLENGTH": 0x6700,
        "ERR_NOINFO68": 0x6800,
        "ERR_CHANNELNOTSUPPORTED": 0x6881,
        "ERR_SECMESSNOTSUPPORTED": 0x6882,
        "ERR_LASTCMDEXPECTED": 0x6883,
        "ERR_CHAININGNOTSUPPORTED": 0x6884,
        "ERR_NOINFO69": 0x6900,
        "ERR_INCOMPATIBLEWITHFILE": 0x6981,
        "ERR_SECSTATUS": 0x6982,
        "ERR_AUTHBLOCKED": 0x6983,
        "ERR_REFNOTUSABLE": 0x6984,
        "ERR_CONDITIONNOTSATISFIED": 0x6985,
        "ERR_NOCURRENTEF": 0x6986,
        "ERR_SECMESSOBJECTSMISSING": 0x6987,
        "ERR_SECMESSOBJECTSINCORRECT": 0x6988,
        "ERR_NOINFO6A": 0x6A00,
        "ERR_INCORRECTPARAMETERS": 0x6A80,
        "ERR_NOTSUPPORTED": 0x6A81,
        "ERR_FILENOTFOUND": 0x6A82,
        "ERR_RECORDNOTFOUND": 0x6A83,
        "ERR_NOTENOUGHMEMORY": 0x6A84,
        "ERR_NCINCONSISTENTWITHTLV": 0x6A85,
        "ERR_INCORRECTP1P2": 0x6A86,
        "ERR_NCINCONSISTENTP1P2": 0x6A87,
        "ERR_DATANOTFOUND": 0x6A88,
        "ERR_FILEEXISTS": 0x6A89,
        "ERR_DFNAMEEXISTS": 0x6A8A,
        "ERR_OFFSETOUTOFFILE": 0x6B00,
        "ERR_INSNOTSUPPORTED": 0x6D00,
}

SW_MESSAGES = {
    0x9000: 'Normal processing (No further qualification)',

    0x6200: 'Warning processing (State of non-volatile memory is unchanged): '
            'No information given',
    0x6281: 'Warning processing (State of non-volatile memory is unchanged): '
            'Part of returned data may be corrupted',
    0x6282: 'Warning processing (State of non-volatile memory is unchanged): '
            'End of file or record reached before reading Ne bytes',
    0x6283: 'Warning processing (State of non-volatile memory is unchanged): '
            'Selected file deactivated',
    0x6284: 'Warning processing (State of non-volatile memory is unchanged): '
            'File control information not formatted according to 5.3.3',
    0x6285: 'Warning processing (State of non-volatile memory is unchanged): '
            'Selected file in termination state',
    0x6286: 'Warning processing (State of non-volatile memory is unchanged): '
            'No input data available from a sensor on the card',

    0x6300: 'Warning processing (State of non-volatile memory has changed): '
            'No information given',
    0x6381: 'Warning processing (State of non-volatile memory has changed): '
            'File filled up by the last write',

    0x6400: 'Execution error (State of non-volatile memory is unchanged): '
            'Execution error',
    0x6401: 'Execution error (State of non-volatile memory is unchanged): '
            'Immediate response required by the card',

    0x6500: 'Execution error (State of non-volatile memory has changed): '
            'No information given',
    0x6581: 'Execution error (State of non-volatile memory has changed): '
            'Memory failure',

    0x6700: 'Checking error: Wrong length; no further indication',

    0x6800: 'Checking error (Functions in CLA not supported): '
            'No information given',
    0x6881: 'Checking error (Functions in CLA not supported): '
            'Logical channel not supported',
    0x6882: 'Checking error (Functions in CLA not supported): '
            'Secure messaging not supported',
    0x6883: 'Checking error (Functions in CLA not supported): '
            'Last command of the chain expected',
    0x6884: 'Checking error (Functions in CLA not supported): '
            'Command chaining not supported',

    0x6900: 'Checking error (Command not allowed): '
            'No information given',
    0x6981: 'Checking error (Command not allowed): '
            'Command incompatible with file structure',
    0x6982: 'Checking error (Command not allowed): '
            'Security status not satisfied',
    0x6983: 'Checking error (Command not allowed): '
            'Authentication method blocked',
    0x6984: 'Checking error (Command not allowed): '
            'Reference data not usable',
    0x6985: 'Checking error (Command not allowed): '
            'Conditions of use not satisfied',
    0x6986: 'Checking error (Command not allowed): '
            'Command not allowed (no current EF)',
    0x6987: 'Checking error (Command not allowed): '
            'Expected secure messaging data objects missing',
    0x6988: 'Checking error (Command not allowed): '
            'Incorrect secure messaging data objects',

    0x6A00: 'Checking error (Wrong parameters P1-P2): '
            'No information given',
    0x6A80: 'Checking error (Wrong parameters P1-P2): '
            'Incorrect parameters in the command data field',
    0x6A81: 'Checking error (Wrong parameters P1-P2): '
            'Function not supported',
    0x6A82: 'Checking error (Wrong parameters P1-P2): '
            'File or application not found',
    0x6A83: 'Checking error (Wrong parameters P1-P2): '
            'Record not found',
    0x6A84: 'Checking error (Wrong parameters P1-P2): '
            'Not enough memory space in the file',
    0x6A85: 'Checking error (Wrong parameters P1-P2): '
            'Nc inconsistent with TLV structure',
    0x6A86: 'Checking error (Wrong parameters P1-P2): '
            'Incorrect parameters P1-P2',
    0x6A87: 'Checking error (Wrong parameters P1-P2): '
            'Nc inconsistent with parameters P1-P2',
    0x6A88: 'Checking error (Wrong parameters P1-P2): '
            'Referenced data or reference data not found (exact meaning '
            'depending on the command)',
    0x6A89: 'Checking error (Wrong parameters P1-P2): '
            'File already exists',
    0x6A8A: 'Checking error (Wrong parameters P1-P2): '
            'DF name already exists',

    0x6B00: 'Checking error (Wrong parameters P1-P2): '
            'Wrong parameters (offset outside the EF)',
    0x6D00: 'Checking error (Instruction code not supported or invalid)',
    0x6E00: 'Checking error (Class not supported)',
    0x6F00: 'Checking error (No precise diagnosis)',
}
for i in range(0, 0xff):
    SW_MESSAGES[0x6100 + i] = 'Normal Processing (' + str(i) + \
                              ' data bytes still available)'
    SW_MESSAGES[0x6600 + i] = 'Execution error (Security-related issues)'
    SW_MESSAGES[0x6C00 + i] = 'Checking error (Wrong Le field; ' + str(i) + \
                              ' available data bytes)'


class SwError(Exception):
    def __init__(self, sw):
        self.sw = sw
        self.message = SW_MESSAGES[sw]
