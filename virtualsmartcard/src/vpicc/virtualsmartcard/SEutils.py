#
# Copyright (C) 2009 Dominik Oepen
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
import TLVutils
from time import time
from random import seed, randint
from virtualsmartcard.ConstantDefinitions import CRT_TEMPLATE, ALGO_MAPPING
from virtualsmartcard.utils import inttostring
from virtualsmartcard.SWutils import SwError, SW

class ControlReferenceTemplate:
    """
    Control Reference Templates are used to configure the Security Environments.
    They specifiy which algorithms to use in which mode of operation and with
    which keys. There are six different types of Control Reference Template:
    HT, AT, KT, CCT, DST, CT-sym, CT-asym.
    """
    def __init__(self, tag, config=""):
        """
        Generates a new CRT
        @param type: Type of the CRT (HT, AT, KT, CCT, DST, CT-sym, CT-asym) 
        @param config: A string containing TLV encoded Security Environment
                       parameters 
        """
        if tag not in (CRT_TEMPLATE["AT"], CRT_TEMPLATE["HT"],
                        CRT_TEMPLATE["KAT"], CRT_TEMPLATE["CCT"],
                        CRT_TEMPLATE["DST"], CRT_TEMPLATE["CT"]):
            raise ValueError, "Unknown control reference tag."
        else:
            self.type = tag

        self.iv = None
        self.keyref = None
        self.key = None
        self.fileref = None
        self.DFref = None
        self.keylength = None
        self.algorithm = None
        self.usage_qualifier = None
        if config != "":
            self.parse_SE_config(config)
        self.__config_string = config
        
    def parse_SE_config(self, config):
        """
        Parse a control reference template as given e.g. in an MSE APDU.
        @param config : a TLV string containing the configuration for the CRT. 
        """
        
        structure = TLVutils.unpack(config)
        for tlv in structure:
            tag, length, value = tlv    
            if tag == 0x80:
                self.__set_algo(value)
            elif tag in (0x81, 0x82, 0x83, 0x84):
                self.__set_key(tag, value)
            elif tag in range(0x85, 0x93):
                self.__set_iv(tag, length, value)
            elif tag == 0x95:
                self.usage_qualifier = value
            
    def __set_algo(self, data):
        """
        Set the algorithm to be used by this CRT. The algorithms are specified
        in a global dictionary. New cards may add or modify this table in order
        to support new or different algorithms.
        @param data: reference to an algorithm
        """
        if not ALGO_MAPPING.has_key(data):
            raise SwError(SW["ERR_REFNOTUSABLE"]) 
        else:
            #TODO: Sanity checking
            self.algorithm = ALGO_MAPPING[data]
            self.__replace_tag(0x80, data)
    
    def __set_key(self, tag, value):
        if tag == 0x81:
            self.fileref = value
        elif tag == 0x82:
            self.DFref = value
        elif tag in (0x83, 0x84):
            #Todo: Resolve reference
            self.keyref = value
    
    def __set_iv(self, tag, length, value):
        iv = None
        if tag == 0x85:
            iv = 0x00
        elif tag == 0x86:
            pass #What is initial chaining block?
        elif tag == 0x87 or tag == 0x93:
            if length != 0:
                iv = value
            else:
                iv = self.iv + 1
        elif tag == 0x91:
            if length != 0:
                iv = value
            else:
                seed(time())
                iv = hex(randint(0, 255))
        elif tag == 0x92:
            if length != 0:
                iv = value
            else:
                iv = int(time())
        self.iv = iv
       
    def __replace_tag(self, tag, data):
        """
        Adjust the config string using a given tag, value combination. If the
        config string already contains a tag, value pair for the given tag,
        replace it. Otherwise append tag, length and value to the config string.
        """
        position = 0
        while self.__config_string[position:position+1] != tag and position < len(self.__config_string):    
            length = inttostring(self.__config_string[position+1:position+2])
            position += length + 3

        if position < len(self.__config_string): #Replace Tag
            length = inttostring(self.__config_string[position+1:position+2])
            self.__config_string = self.__config_string[:position] + tag +\
                                   inttostring(len(data)) + data +\
                                   self.__config_string[position+2+length:]
        else: #Add new tag
            self.__config_string += tag + inttostring(len(data)) + data
    
    def to_string(self):
        """
        Return the content of the CRT, encoded as TLV data in a string
        """
        return self.__config_string
    
    def __str__(self):
        return self.__config_string
    
    
