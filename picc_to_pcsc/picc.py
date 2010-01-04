#!/usr/bin/python

"""
 * Copyright (C) 2009 Dominik Oepen
 *
 * This file is part of picc.
 *
 * picc is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * picc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * picc.  If not, see <http://www.gnu.org/licenses/>.
"""

import sys
import smartcard.util
from optparse import OptionParser
from smartcard.System import readers
from struct import pack

def setup(readernumber):
	"""Open up a connection to the specified reader via PCSC
	"""

	r = readers()
	connection = r[readernumber].createConnection()
	connection.connect()
	return connection

def read_from_picc(fd):
	"""Read a line from the terminal, parse it and return a string 
	   representation	
	"""

	line = fd.readline()
	#Parse line
	if (line[0] != '0'):
		print(line)
		return None
	else:
		idx = line.find(':')
		if (idx == -1):
			print "Failed to parse string"
			return None
		length = int(line[:idx])
		apdu = line[idx+2:]
		apdu = apdu.replace(' ','')
		#if (len(apdu) != length*2+1):
		#	print "Encountered corrupted APDU"
		#	return None
		return apdu 
	
def transmit_to_pcsc(line, connection):
	"""Write a line (including the string representation of an APDU)
	   to PCSC. Return the response from PCSC or NULL in case of error
	"""

	#Convert string to tuple of Bytes
	try:
		apdu_bytes = smartcard.util.toBytes(line)
	except TypeError, e:
		print "Failed to transmit APDU to PCSC: " + e 
		return #FIXME

	rapdu = connection.transmit( SELECT + DF_TELECOM )
	#print "%x %x" % (sw1, sw2)

	rv = smartcard.util.toHexString(rapdu, PACK)
	
	return rv

def write_to_picc(line,fd):
	"""Read a line and format it for the OpenPICC. Then write it 
	   to the terminal
	"""

	#fd.write("0002: 90 00\r\n")	
	length = len(line)
	length /= 2
	output_string = hex(length)[2:]
	output_string += ":"
	
	for (i in range[0, len(line), 2])
		output_string += " " + line[i:i+2]

	output_string += "\r\n"
	fd.write(output_string)
	#fd.flush()

if __name__ == "__main__":

	#Parse command line arguments
	parser = OptionParser()
	parser.add_option("-t", "--tty", action="store", dest="ttyno",
			type="string", default='0',
            help="Number of ttyACM the OpenPICC is connected to [default: %default]")
	parser.add_option("-r", "--reader", action="store", dest="readerno",
			type="string", default='0',
            help="Number of ttyACM the OpenPICC is connected to [default: %default]")
	(options, args) = parser.parse_args()
	if (options.ttyno.isdigit()):
		nr = options.ttyno
	else:
		print "Invalid tty number %s. Using default tty (/dev/ttyACM0)" % options.ttyno
		nr = '0'
	if (options.readerno.isdigit()):
		readerno = int(options.readerno)
	else:
		print "Invalid reader number %s. Using default reader (0)"
		readerno = 0
	ttyPath = "/dev/ttyACM" + nr

	#Connect to Virtual PCD via PC/SC
	try:
		connection = setup(readerno)
	except:
		print "PC/SC Error"
		sys.exit()

	#Open terminal
	try:
		fd = open(ttyPath,'a+')
	except IOError:
		print "Failed to open %s" % ttyPath
		sys.exit()

	#Actual processing
	while (True):

		try:
			apdu = read_from_picc(fd)
		except IOError:
			print "Failed to read from %s" % ttyPath
			sys.exit()	
		if (apdu == None): 
			continue 
		else: 
			print("Got APDU: %s" %apdu)

		response = transmit_to_pcsc(apdu)

		try:
			write_to_picc(response,fd)
		except IOError:
			print "Failed to write to %s" % ttyPath
			sys.exit()
