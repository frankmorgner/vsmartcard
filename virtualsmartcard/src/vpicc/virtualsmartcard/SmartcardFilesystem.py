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

# TODO: use bertlv_pack for fdm
# TODO: zu lange daten abschneiden und trotzdem tlv laenge beibehalten

from pickle import dumps, loads
import logging
from virtualsmartcard.ConstantDefinitions import DCB, FDB, FID, LCB, REF
from virtualsmartcard.TLVutils import *
from virtualsmartcard.SWutils import SW, SwError
from virtualsmartcard.utils import stringtoint, inttostring, hexdump


def isEqual(list):
    """Returns True, if all items are equal, otherwise False"""
    if len(list) > 1:
        for item in list:
            if item != list[0]:
                return False

    return True


def walk(start, path):
    """Walks a path of fids and returns the last file (EF or DF).

    :param start: DF, where to look for the first fid
    :param path:  a string of fids
    """
    if len(path) % 2 != 0:
        raise SwError(SW["ERR_INCORRECTPARAMETERS"])
    index = 0
    while index + 2 <= len(path):
        if not isinstance(start, DF):
            # File or application not found
            raise SwError(SW["ERR_FILENOTFOUND"])
        start = start.select('fid', stringtoint(path[index:index+2]))
        index = index + 2

    return start


def getfile_byrefdataobj(mf, refdataobjs):
    """Returns a list of files according to the given list of reference data
    objects.

    param mf:          the MF
    param refdataobjs: a list of 3-tuples (tag, length, newvalue)"""

    result = [None]
    for tag, length, newvalue in refdataobjs:
        if tag != TAG["FILE_REFERENCE"]:
            raise ValueError

        if length == 0:
            file = mf

        elif length <= 2:
            newvalue = stringtoint(newvalue)
            if length == 1:
                if (newvalue & 5 != 0 or (newvalue >> 3) == 0 or
                        (newvalue >> 3) == (0xff >> 3)):
                    raise SwError(SW["ERR_INCORRECTPARAMETERS"])
                else:
                    file = mf.select('shortfid', newvalue >> 3)

            else:
                # length == 2:
                file = mf.select('fid', newvalue)

        else:
            if length % 2 == 1:
                raise NotImplementedError
            else:
                if newvalue[:2] == "\x3f\x00":
                    # absolute
                    file = walk(mf, newvalue[2:])
                else:
                    # relative
                    file = walk(mf.currentDF(), newvalue[2:])

        result.append(file)

    return result


def write(old, newlist, offsets, datacoding, maxsize=None):
    """Returns the status bytes and the result of a write operation according to
    the given data coding.

    :param old: string of old data
    :param newlist: a list of new data string
    :param offsets: a list of offsets, each for one new data strings
    :param datacoding: DCB["ONETIMEWRITE"] (replace) or DCB["WRITEOR"]
                       (logical or) or DCB["WRITEAND"] (logical and) or
                       DCB["PROPRIETARY"] (logical xor)
    :param maxsize: the maximum number of bytes in the result
    """
    result = old
    listindex = 0
    while listindex < len(offsets) and listindex < len(newlist):
        offset = offsets[listindex]
        new = newlist[listindex]
        writenow = len(new)

        if offset > len(result):
            raise SwError(SW["ERR_OFFSETOUTOFFILE"])
        if maxsize and offset + writenow > maxsize:
            raise SwError(SW["ERR_NOTENOUGHMEMORY"], old)

        if datacoding == DCB["ONETIMEWRITE"]:
            result = (result[0:offset] +
                      new[0:writenow] +
                      result[offset + writenow:len(result)])

        else:
            if offset + writenow > len(old):
                raise SwError(SW["ERR_NOTENOUGHMEMORY"], old)

            newindex = 0
            resultindex = offset + newindex
            while newindex < writenow:
                if datacoding == DCB["WRITEOR"]:
                    newpiece = inttostring(
                            result[resultindex] | new[newindex])
                elif datacoding == DCB["WRITEAND"]:
                    newpiece = inttostring(
                            result[resultindex] & new[newindex])
                elif datacoding == DCB["PROPRIETARY"]:
                    # we use it for XOR
                    newpiece = inttostring(
                            result[resultindex] ^ new[newindex])
                result = (result[0:resultindex] + newpiece +
                          result[resultindex+1:len(result)])
                newindex = newindex + 1
                resultindex = resultindex + 1

        listindex = listindex + 1

    return result


def get_indexes(items, reference=REF["IDENTIFIER_FIRST"], index_current=0):
    """
    Returns all indexes of the list, which are specified by 'reference' and by
    the current index 'index_current' (-1 for no current item) in the correct
    order. I. e.:

    REF["IDENTIFIER_FIRST"]: all indexes from first to the last item
    REF["IDENTIFIER_LAST"]: all indexes from the last to first item
    REF["IDENTIFIER_NEXT"]: all indexes from the next to the last item
    REF["IDENTIFIER_PREVIOUS"]: all indexes from the previous to the first item
    """
    if (reference in [REF["IDENTIFIER_FIRST"], REF["IDENTIFIER_LAST"]] or
            index_current == -1):
        # Read first occurrence OR
        # Read last occurrence OR
        # No current record (and next/previous occurrence)
        indexes = range(0, len(items))
        if (reference == REF["IDENTIFIER_LAST"] or
                reference == REF["IDENTIFIER_PREVIOUS"]):
            # Read last occurrence OR
            # No current record and previous occurrence
            indexes.reverse()
    elif reference == REF["IDENTIFIER_PREVIOUS"]:
        # Read previous occurrence
        indexes = range(0, index_current)
        indexes.reverse()
    else:
        # Read next occurrence
        indexes = range(index_current + 1, len(items))
    return indexes


def prettyprint_anything(indent, thing):
    """
    Returns a recursively generated string representation of an object and its
    attributes.
    """
    s = "%s{%s at 0x%x}:" % (indent, thing.__class__.__name__, id(thing))
    indent = indent + "  "
    for (attribute, newvalue) in thing.__dict__.items():
        if isinstance(newvalue, int):
            s = s + "\n" + indent + attribute + (16-len(attribute)) * " " +\
                "0x%x" % (newvalue)
        elif isinstance(newvalue, str):
            s = s + "\n" + indent + attribute + (16-len(attribute)) * " " +\
                "length %d:" % len(newvalue)
            s = s + "\n" + indent + hexdump(newvalue, len(indent))
        elif isinstance(newvalue, list):
            s = s + "\n" + indent + attribute + (16 - len(attribute)) * " "
            for item in newvalue:
                if attribute == '_named_dfs':
                    s = s + "\n" + "%s%s" % (indent + "  ", hexdump(item.dfname, short=True))
                else:
                    s = s + "\n" + prettyprint_anything(indent + "  ", item)
    return s


def make_property(prop, doc):
    """
    Assigns a property to the calling object. This is used to decorate instance
    variables with docstrings.
    """
    return property(
            lambda self:        getattr(self, "_"+prop),
            lambda self, value: setattr(self, "_"+prop, value),
            lambda self:        delattr(self, "_"+prop),
            doc)

class File(object):
    """Template class for a smartcard file."""
    bertlv_data = make_property("bertlv_data", "list of (tag, length, "
                                               "value)-tuples of BER-TLV "
                                               "coded data objects "
                                               "(encrypted)")
    lifecycle = make_property("lifecycle", "life cycle byte")
    parent = make_property("parent ", "parent DF")
    fid = make_property("fid", "file identifier")
    filedescriptor = make_property("filedescriptor", "file descriptor byte")
    simpletlv_data = make_property("simpletlv_data", "list of (tag, length, "
                                                     "value)-tuples of "
                                                     "SIMPLE-TLV coded data "
                                                     "objects (encrypted)")

    def __init__(self, parent, fid, filedescriptor,
                 lifecycle=LCB["ACTIVATED"],
                 simpletlv_data=None,
                 bertlv_data=None,
                 SAM=None,
                 extra_fci_data=b''):
        """
        The constructor is supposed to be involved by creation of a DF or EF.
        """
        if (fid > 0xFFFF or fid < 0 or fid in
                [FID["PATHSELECTION"], FID["RESERVED"]] or
                filedescriptor > 0xFF or lifecycle > 0xFF):
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        self.lifecycle = lifecycle
        self.parent = parent
        if parent:
            if parent.fid == fid:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        self.fid = fid
        self.filedescriptor = filedescriptor
        self.SAM = SAM
        self.extra_fci_data = extra_fci_data
        if simpletlv_data:
            if not isinstance(simpletlv_data, list):
                raise TypeError("must be a list of (tag, length, "
                                "value)-tuples")
            self.simpletlv_data = simpletlv_data
        if bertlv_data:
            if not isinstance(bertlv_data, list):
                raise TypeError("must be a list of (tag, length, "
                                "value)-tuples")
            self.bertlv_data = bertlv_data

    def decrypt(self, path, data):
        if self.SAM is None:  # WARNING: Fails silent
            return data
        else:
            return self.SAM.FSencrypt(path, data)

    def encrypt(self, path, data):
        if self.SAM is None:  # WARNING: Fails silent
            return data
        else:
            return self.SAM.FSdecrypt(path, data)

    def __str__(self):
        """Returns a string of the object using an prettyprint_anything."""
        return prettyprint_anything("", self)
    __repr__ = __str__

    def getpath(self):
        """Returns the path to this file beginning with the MF's fid."""
        if self.parent is None:
            return inttostring(self.fid, 2)
        else:
            return self.parent.getpath() + inttostring(self.fid, 2)

    def getMF(self):
        """Return MF of the card that contains this file"""
        if self.parent is None:
            return self
        else:
            return self.parent.getMF()

    def getdata(self, isSimpleTlv, requestedTL):
        """
        Returns a string of either the file's BER-TLV or the file's SIMPLE-TLV
        coded data objects depending on the bool 'isSimpleTlv'. 'requestedTL'
        is a list of (tag, length)-tuples that specify which tags should be
        returned in what size.
        """
        if isSimpleTlv:
            attribute = 'simpletlv_data'
        else:
            attribute = 'bertlv_data'

        if not hasattr(self, attribute):
            raise SwError(SW["ERR_NOTSUPPORTED"])

        tlv_data = getattr(self, attribute)
        if requestedTL == []:
            result = tlv_data
        else:
            result = []
            for T, L in requestedTL:
                tagfound = False
                for i in range(0, len(tlv_data)):
                    tag, _, value = tlv_data[i]
                    if T == tag:
                        tagfound = True
                        if L == 0:
                            result.append(tlv_data[i])
                        else:
                            result.append((T, L, value[:L]))
                if not tagfound:
                    raise SwError(SW["ERR_DATANOTFOUND"])

        if isSimpleTlv:
            return simpletlv_pack(result)
        return bertlv_pack(result)

    def putdata(self, isSimpleTlv, newtlvlist):
        """
        Sets either the file's BER-TLV or the file's SIMPLE-TLV coded data
        objects depending on the bool 'isSimpleTlv'. 'newtlvlist' is a list of
        (tag, length, value)-tuples of new data.
        """
        if isSimpleTlv:
            attribute = 'simpletlv_data'
        else:
            attribute = 'bertlv_data'

        if not hasattr(self, attribute):
            raise SwError(SW["ERR_NOTSUPPORTED"])

        tlv_data = getattr(self, attribute)
        for tag, newlength, newvalue in newtlvlist:
            tagfound = False
            for i in range(0, len(tlv_data)):
                t, oldlength, oldvalue = tlv_data[i]
                if t == tag:
                    # TODO: what if multiple tags can be found?
                    value = write(oldvalue, [newvalue], [0], newlength,
                                  self.datacoding)
                    tlv_data[i] = (tag, len(value), value)
                    tagfound = True
            if not tagfound:
                tlv_data.append(tag, newlength, newvalue)
        setattr(self, attribute, tlv_data)

    def readbinary(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])

    def writebinary(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])

    def updatebinary(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])

    def erasebinary(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])

    def readrecord(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])

    def writerecord(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])

    def appendrecord(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])

    def updaterecord(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])

    def select(self, *argz, **args):
        """Only a template, will raise an error."""
        raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])


class DF(File):
    """Class for a dedicated file"""
    data = make_property("data", "unknown")
    content = make_property("content", "list of files of the DF")
    dfname = b"" # make_property("dfname", "string with up to 16 bytes. DF name,"
                #                     "which can also be used as application"
                #                     "identifier.")

    def __init__(self, parent, fid,
                 filedescriptor=FDB["NOTSHAREABLEFILE"] | FDB["DF"],
                 lifecycle=LCB["ACTIVATED"],
                 simpletlv_data=None, bertlv_data=None, dfname=None, data="",
                 extra_fci_data=b''):
        """
        See File for more.
        """
        File.__init__(self, parent, fid, filedescriptor, lifecycle,
                      simpletlv_data, bertlv_data, None, extra_fci_data)
        if dfname:
            if len(dfname) > 16:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])
            self.dfname = dfname
        self.content = []
        # TODO: opensc sends the length of data, but what does it limit
        # (bertlv-data/simpletlv-data/number of files in DF, ...)?
        self.data = data

    def __len__(self):
        """
        x.__len__() <==> len(x.content)
        """
        return len(self.content)

    def __getitem__(self, key):
        """
        x.__getitem__(key) <==> x.content[key]
        """
        return self.content[key]

    def __setitem__(self, key, value):
        """
        x.__setitem__(key, value) <==> x.content[key]=value
        """
        return self.content[key]

    def __delitem__(self, key):
        """
        x.__delitem__(key) <==> del x.content[key]
        """
        del self.content[key]

    def __contains__(self, item):
        """
        x.__contains__(item) <==> item in x.content
        """
        return item in self.content

    def append(self, file):
        """Appends 'file' to the content of the DF."""
        if not (isinstance(file, DF) or isinstance(file, EF)):
            raise TypeError

        if (self.fid == file.fid or file.fid == FID["MF"] or file.fid ==
                FID["RESERVED"] or file.fid == FID["PATHSELECTION"]):
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        for f in self.content:
            if f.fid == file.fid:
                raise SwError(SW["ERR_FILEEXISTS"])
            if (hasattr(f, 'shortfid') and hasattr(file, 'shortfid') and
                    f.shortfid == file.shortfid):
                raise SwError(SW["ERR_FILEEXISTS"])

        if (hasattr(file, 'dfname')):
            mf = self.getMF()
            for f in mf.named_dfs:
                if (f.dfname == file.dfname):
                    raise SwError(SW["ERR_DFNAMEEXISTS"])
            mf.named_dfs.append(file)
        
        self.content.append(file)

    def select(self, attribute, value, reference=REF["IDENTIFIER_FIRST"],
               index_current=0):
        """
        Returns the first file of the DF, that has the 'attribute' with the
        specified 'value'. For partial DF name selection you must specify the
        first/last/next or previous occurence with 'reference' and the index of
        the current file 'index_current' (-1 for None).
        """
        if attribute == 'dfname':
            mf = self.getMF()
            search_in = mf.named_dfs
        else:
            search_in = self.content
            
        indexes = get_indexes(search_in, reference, index_current)

        for i in indexes:
            file = search_in[i]
            if (hasattr(file, attribute) and
                    ((getattr(file, attribute) == value) or
                        (attribute == 'dfname' and
                            getattr(file, attribute).startswith(value)))):
                return file
        # not found
        if isinstance(value, int):
            logging.debug("file (%s=%x) not found in:\n%s" %
                          (attribute, value, self.fid))
        elif isinstance(value, str):
            logging.debug("file (%s=%r) not found in:\n%s" %
                          (attribute, value, self.fid))
        raise SwError(SW["ERR_FILENOTFOUND"])

    def remove(self, file):
        """Removes 'file' from the content of the DF"""
        self.content.remove(file)
        if (hasattr(file, 'dfname')):
            mf = self.getMF()
            mf.named_dfs.remove(file)


class MF(DF):
    """Class for a master file"""
    current = make_property("current", "the currently selected file")
    firstSFT = make_property("firstSFT", "string of length 1. The first"
                                         "software function table from the"
                                         "historical bytes.")
    secondSFT = make_property("secondSFT", "string of length 1. The second"
                                           "software function table from the"
                                           "historical bytes.")
    named_dfs = make_property("named_dfs", "list of DFs with dfname specified")

    def __init__(self, filedescriptor=FDB["NOTSHAREABLEFILE"] | FDB["DF"],
                 lifecycle=LCB["ACTIVATED"],
                 simpletlv_data=None, bertlv_data=None, dfname=None):
        """The file identifier FID["MF"] is automatically added.

        See DF for more.
        """
        DF.__init__(self, None, FID["MF"], filedescriptor, lifecycle,
                    simpletlv_data, bertlv_data, dfname)
        self.current = self
        self.firstSFT = inttostring(MF.makeFirstSoftwareFunctionTable(), 1)
        self.secondSFT = inttostring(MF.makeSecondSoftwareFunctionTable(), 1)
        self.named_dfs = []
        if dfname:
            self.named_dfs.append(self)

    @staticmethod
    def makeFirstSoftwareFunctionTable(
            DFSelectionByFullDFName=True, DFSelectionByPartialDFName=True,
            DFSelectionByPath=True, DFSelectionByFID=True,
            DFSelectionByApplication_implicite=True, ShortFIDSupported=True,
            RecordNumberSupported=True, RecordIdentifierSupported=True):
        """
        Returns a byte according to the first software function table from the
        historical bytes of the card capabilities.
        """
        fsft = 0
        if DFSelectionByFullDFName:
            fsft |= 1 << 7
        if DFSelectionByPartialDFName:
            fsft |= 1 << 6
        if DFSelectionByPath:
            fsft |= 1 << 5
        if DFSelectionByFID:
            fsft |= 1 << 4
        if DFSelectionByApplication_implicite:
            fsft |= 1 << 3
        if ShortFIDSupported:
            fsft |= 1 << 2
        if RecordNumberSupported:
            fsft |= 1 << 1
        if RecordIdentifierSupported:
            fsft |= 1
        return fsft

    @staticmethod
    def makeSecondSoftwareFunctionTable(DCB=DCB["ONETIMEWRITE"] | 1):
        """
        The second software function table from the historical bytes contains
        the data coding byte.
        """
        return DCB

    def currentDF(self):
        """Returns the current DF."""
        if isinstance(self.current, EF):
            return self.current.parent
        else:
            return self.current

    def currentEF(self):
        """Returns the current EF or None if not available."""
        if isinstance(self.current, EF):
            return self.current
        else:
            return None

    @staticmethod
    def encodeFileControlParameter(file):
        """
        Returns a string of TLV-coded file control information of 'file'. Note:
        The result is not prepended with tag and length for neither TCP, FMD
        nor FCI template.
        """
        fdm = [inttostring(TAG["FILEIDENTIFIER"]) + b"\x02" + inttostring(file.fid, 2),
               inttostring(TAG["LIFECYCLESTATUS"]) + b"\x01" + inttostring(file.lifecycle)]
        fdm.append(file.extra_fci_data)

        # TODO filesize and data objects
        if isinstance(file, EF):
            if hasattr(file, 'shortfid'):
                fdm.append(b"%c\x01%c" % (TAG["SHORTFID"], file.shortfid << 3))
            else:
                fdm.append(b"%c\x00" % TAG["SHORTFID"])

            if isinstance(file, TransparentStructureEF):
                l = inttostring(len(file.data), 2, True)
                fdm.append(b"%c%c%s" % (TAG["BYTES_EXCLUDINGSTRUCTURE"],
                           inttostring(len(l)), l))
                fdm.append(b"%c%c%s" % (TAG["BYTES_INCLUDINGSTRUCTURE"],
                           inttostring(len(l)), l))
                fdm.append(b"%c\x02%c%c" % (TAG["FILEDISCRIPTORBYTE"],
                           file.filedescriptor, file.datacoding))

            elif isinstance(file, RecordStructureEF):
                l = 0
                records = file.records
                for r in records:
                    if file.hasSimpleTlv():
                        l += simpletlv_unpack(r.data)[0][1]
                    else:
                        l += len(r.data)
                fdm.append(b"%c\x02%s" % (TAG["BYTES_EXCLUDINGSTRUCTURE"],
                           inttostring(l, 2, True)))
                fdm.append(b"%c\x02%s" % (TAG["BYTES_INCLUDINGSTRUCTURE"],
                           inttostring(l, 2, True)))
                l = len(records)
                fdm.append(b"%c\x06%c%c%c%c%s" % (TAG["FILEDISCRIPTORBYTE"],
                           file.filedescriptor, file.datacoding,
                           file.maxrecordsize >> 8,
                           file.maxrecordsize & 0x00ff,
                           inttostring(l, 2)))

        elif isinstance(file, DF):
            # TODO number of files == number of data bytes?
            fdm.append(b"%c\x01%c" % (TAG["FILEDISCRIPTORBYTE"],
                       file.filedescriptor))
            if hasattr(file, 'dfname'):
                fdm.append(b"%c%c%s" % (TAG["DFNAME"], len(file.dfname),
                           file.dfname))

        else:
            raise TypeError

        return b"".join(fdm)

    def _selectFile(self, p1, p2, data):
        """
        Returns the file specified by 'p1' and 'data' from the select
        file command APDU.
        """
        P1_FILE = 0x00
        P1_CHILD_DF = 0x01
        P1_CHILD_EF = 0x02
        P1_PARENT_DF = 0x03
        P1_DF_NAME = 0x04
        P1_PATH_FROM_MF = 0x08
        P1_PATH_FROM_CURRENTDF = 0x09

        #if (p1 >> 4) != 0 or 
        if p1 == P1_FILE:
            import binascii
            logging.debug(f"p1 >> 4 or P1_FILE: {binascii.hexlify(data)}")
            # RFU OR
            # When P1='00', the card knows either because of a specific coding
            # of the file identifier or because of the context of execution of
            # the command if the file to select is the MF, a DF or an EF.
            try:
                if data[:2] == inttostring(self.fid):
                    selected = walk(self, data[2:])
                elif data[:2] == inttostring(self.currentDF().fid):
                    selected = walk(self.currentDF(), data[2:])
                else:
                    selected = walk(self.currentDF(), data)
            except SwError as e:
                # If everything fails, look at MF
                selected = walk(self, data)

        elif p1 == P1_CHILD_DF or p1 == P1_CHILD_EF:
            logging.debug("P1_CHILD_DF or P1_CHILD_EF")
            selected = self.currentDF().select('fid', stringtoint(data))
            if ((p1 == P1_CHILD_DF and not isinstance(selected, DF)) or
                    (p1 == P1_CHILD_EF and not isinstance(selected, EF))):
                # Command incompatible with file structure
                raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])
        elif p1 == P1_PATH_FROM_MF:
            logging.debug("P1 PATH FROM MF")
            selected = walk(self, data)
        elif p1 == P1_PATH_FROM_CURRENTDF:
            logging.debug("P1 PATH FROM CURRENT DF")
            selected = walk(self.currentDF(), data)
        elif p1 == P1_PARENT_DF:
            logging.debug("P1 PARENT DF")
            selected = self.current.parent
        elif p1 == P1_DF_NAME:
            logging.debug("P1 DF NAME")
            df = self.currentDF()
            if df == self or df not in self.content:
                index_current = -1
            else:
                index_current = self.content.index(df)
            selected = self.select('dfname', data,
                                   p2 & REF["REFERENCE_CONTROL_SELECT"],
                                   index_current)
        else:
            logging.debug("unknown selection method: p1 =%s" % p1)
            selected = None

        if selected is None:
            raise SwError(SW["ERR_FILENOTFOUND"])

        return selected

    def selectFile(self, p1, p2, data):
        """
        Function for instruction 0xa4. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        P2_FCI = 0
        P2_FCP = 1 << 2
        P2_FMD = 2 << 2
        P2_NONE = 3 << 2
        file = self._selectFile(p1, p2, data)

        if p2 == P2_NONE:
            data = b""
        elif p2 == P2_FMD:
            # TODO
            data = b""
        else:
            if p2 == P2_FCP:
                tag = TAG["FILECONTROLPARAMETERS"]
            else:
                tag = TAG["FILECONTROLINFORMATION"]
            fdm = MF.encodeFileControlParameter(file)
            data = bertlv_pack([(tag, len(fdm), fdm)])

        self.current = file

        return SW["NORMAL"], data

    def dataUnitsDecodePlain(self, p1, p2, data):
        """
        Decodes 'p1', 'p2' and 'data' from a data unit command (i. e.
        read/write/update/search/erase binary) with *even* instruction code.

        :returns: the specified TransparentStructureEF, a list of offsets and a
            list of data strings.
        """
        if p1 >> 7:
            # If bit 8 of INS is set to 1, then bits 7
            # and 6 of P1 are set to 00 (RFU), bits 3 to 1 of P1 encode a
            # short EF identifier and P2 (eight bits) encodes an offset
            # from zero to 255.
            ef = self.currentDF().select('shortfid', p1 & 0x1f)
            self.current = ef
            offsets = [p2]
        else:
            # If bit 1 of INS is set to 0 and bit 8 of P1 to 0, then P1-P2
            # (fifteen bits) encodes an offset from zero to 32 767.
            ef = self.currentEF()
            if not ef:
                raise SwError(SW["ERR_NOCURRENTEF"])
            offsets = [(p1 << 8) + p2]

        return ef, offsets, [data]

    def dataUnitsDecodeEncapsulated(self, p1, p2, data):
        """
        Decodes 'p1', 'p2' and 'data' from a data unit command (i. e.
        read/write/update/search/erase binary) with *odd* instruction code.

        :returns the specified TransparentStructureEF, a list of offsets and a
            list of data strings.
        """
        # If bit 1 of INS is set to 1, then P1-P2 shall identify an EF. If
        # the first eleven bits of P1-P2 are set to 0 and if bits 5 to 1 of
        # P2 are not all equal and if the card and / or the EF supports
        # selection by short EF identifier, then bits 5 to 1 of P2 encode a
        # short EF identifier (a number from one to thirty). Otherwise,
        # P1-P2 is a file identifier. P1-P2 set to '0000' identifies the
        # current EF. At least one offset data object with tag '54' shall
        # be present in the command data field. When present in a command
        # or response data field, data shall be encapsulated in a
        # discretionary data object with tag '53' or '73'.
        tlv_data = bertlv_unpack(data)
        offsets = decodeOffsetDataObjects(tlv_data)
        datalist = decodeDiscretionaryDataObjects(tlv_data)

        if p1 == 0 and p2 >> 5 == 0:
            ef = self.currentDF().select('shortfid', p2)
        else:
            ef = self.currentDF().select('fid', p1 << 8 + p1)
        self.current = ef

        return ef, offsets, datalist

    def readBinaryPlain(self, p1, p2, data):
        """
        Function for instruction 0xb0. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, offsets, datalist = self.dataUnitsDecodePlain(p1, p2, data)

        result = ef.readbinary(offsets[0])
        return SW["NORMAL"], result

    def readBinaryEncapsulated(self, p1, p2, data):
        """
        Function for instruction 0xb1. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, offsets, datalist = self.dataUnitsDecodeEncapsulated(p1, p2, data)
        result = ef.readbinary(offsets[0])

        r = encodeDiscretionaryDataObjects([result])

        return SW["NORMAL"], r

    def writeBinaryPlain(self, p1, p2, data):
        """
        Function for instruction 0xd0. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, offsets, datalist = self.dataUnitsDecodePlain(p1, p2, data)
        ef.writebinary(offsets, datalist)

        return SW["NORMAL"], b""

    def writeBinaryEncapsulated(self, p1, p2, data):
        """
        Function for instruction 0xd1. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string. Returns the status bytes as two
        byte long integer and the response data as binary string.
        """
        ef, offsets, datalist = self.dataUnitsDecodeEncapsulated(p1, p2, data)
        ef.writebinary(offsets, datalist)

        return SW["NORMAL"], b""

    def updateBinaryPlain(self, p1, p2, data):
        """
        Function for instruction 0xd6. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, offsets, datalist = self.dataUnitsDecodePlain(p1, p2, data)
        ef.updatebinary(offsets, datalist)

        return SW["NORMAL"], b""

    def updateBinaryEncapsulated(self, p1, p2, data):
        """
        Function for instruction 0xd7. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, offsets, datalist = self.dataUnitsDecodeEncapsulated(p1, p2, data)
        ef.updatebinary(offsets, datalist)

        return SW["NORMAL"], b""

    def searchBinaryPlain(self, p1, p2, data):
        ef, offsets, datalist = self.dataUnitsDecodePlain(p1, p2, data)
        r = self.data.find(datalist[0], offsets[0])
        if r == -1:
            raise SwError(SW["ERR_DATANOTFOUND"])

        return SW["NORMAL"], inttostring(r)

    def searchBinaryEncapsulated(self, p1, p2, data):
        ef, offsets, datalist = self.dataUnitsDecodeEncapsulated(p1, p2, data)
        if len(offsets) != len(datalist):
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        result = []
        for data, offset in offsets, datalist:
            r = self.data.find(data, offset)
            if r == -1:
                raise SwError(SW["ERR_DATANOTFOUND"])
            result.append(inttostring(r))

        return SW["NORMAL"], encodeDataOffsetObjects(result)

    def eraseBinaryPlain(self, p1, p2, data):
        """
        Function for instruction 0x0e. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
                  data as binary string.
        """
        ef, offsets, datalist = self.dataUnitsDecodePlain(p1, p2, data)
        # If INS = '0E', then, if present, the command data field encodes
        # the offset of the first data unit not to be erased. This offset
        # shall be higher than the one encoded in P1-P2. If the data field
        # is absent, then the command erases up to the end of the file.
        erasefrom = offsets[0]

        if data:
            tlv_data = bertlv_unpack(data)
            eraseto = decodeOffsetDataObjects(tlv_data)[0]
        else:
            eraseto = None

        ef.erasebinary(erasefrom, eraseto)
        return SW["NORMAL"], b""

    def eraseBinaryEncapsulated(self, p1, p2, data):
        """
        Function for instruction 0x0f. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, offsets, datalist = self.dataUnitsDecodeEncapsulated(p1, p2, data)
        # If INS = '0F', then, if present, the command data field shall
        # consist of zero, one or two offset data objects.  If there is no
        # offset, then the command erases all the data units in the file.
        # If there is one offset, it indicates the first data unit to be
        # erased; then the command erases up to the end of the file. Two
        # offsets define a sequence of data units: the second offset
        # indicates the first data unit not to be erased; it shall be
        # higher than the first offset.
        if len(offsets) > 1:
            eraseto = offsets[1]
        else:
            eraseto = None

        if len(offsets) > 0:
            erasefrom = offsets[0]
        else:
            erasefrom = None

        ef.erasebinary(erasefrom, eraseto)
        return SW["NORMAL"], b""

    def recordHandlingDecode(self, p1, p2):
        """
        Decodes 'p1' and 'p2' from a record handling command (i. e.
        read/write/update/append/search/erase record).

        :returns: the specified RecordStructureEF, the number or identifier of
            the record and a reference, that specifies which record to select
            (i. e. the last 3 bits of 'p1').
        """
        if p1 == 0xff:
            # RFU
            raise SwError(SW["ERR_INCORRECTP1P2"])
        else:
            num_id = p1

        shortfid = p2 >> 3
        if shortfid == 0:
            ef = self.currentEF()
            if ef is None:
                raise SwError(SW["ERR_NOCURRENTEF"])
        elif shortfid == 0x1f:
            # RFU
            raise SwError(SW["ERR_INCORRECTP1P2"])
        else:
            ef = self.currentDF().select('shortfid', shortfid)
            self.current = ef
            ef.resetRecordPointer()

        return ef, num_id, p2 & REF["REFERENCE_CONTROL_RECORD"]

    def readRecordPlain(self, p1, p2, data):
        """
        Function for instruction 0xb2. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, num_id, reference = self.recordHandlingDecode(p1, p2)
        result = ef.readrecord(0, num_id, reference)

        r = b""
        for item in result:
            r += item

        return SW["NORMAL"], r

    def readRecordEncapsulated(self, p1, p2, data):
        """
        Function for instruction 0xb3. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, num_id, reference = self.recordHandlingDecode(p1, p2)
        result = ef.readrecord(0, num_id, reference)

        return SW["NORMAL"], encodeDiscretionaryDataObjects(result)

    def writeRecord(self, p1, p2, data):
        """
        Function for instruction 0xd2. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, num_id, reference = self.recordHandlingDecode(p1, p2)
        if reference not in [REF["IDENTIFIER_FIRST"],
                             REF["IDENTIFIER_LAST"],
                             REF["IDENTIFIER_NEXT"],
                             REF["IDENTIFIER_PREVIOUS"],
                             REF["NUMBER"]]:
            # RFU
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        ef.writerecord(num_id, reference, 1, data)

        return SW["NORMAL"], b""

    def updateRecordPlain(self, p1, p2, data):
        """
        Function for instruction 0xdc. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, num_id, reference = self.recordHandlingDecode(p1, p2)
        if reference not in [REF["IDENTIFIER_FIRST"],
                             REF["IDENTIFIER_LAST"],
                             REF["IDENTIFIER_NEXT"],
                             REF["IDENTIFIER_PREVIOUS"],
                             REF["NUMBER"]]:
            # RFU
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        ef.updaterecord(num_id, reference, 0, data)

        return SW["NORMAL"], b""

    def updateRecordEncapsulated(self, p1, p2, data):
        """
        Function for instruction 0xdd. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, num_id, reference = self.recordHandlingDecode(p1, p2)

        P2_REPLACE = 0x04
        P2_AND = 0x05
        P2_OR = 0x06
        # P2_XOR     = 0x07
        tlv_data = bertlv_unpack(data)
        if reference in [REF["IDENTIFIER_FIRST"],
                         REF["IDENTIFIER_LAST"],
                         REF["IDENTIFIER_NEXT"],
                         REF["IDENTIFIER_PREVIOUS"]]:
            # RFU
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        elif reference == P2_REPLACE:
            ef.writerecord(num_id, reference,
                           decodeOffsetDataObjects(tlv_data)[0],
                           decodeDiscretionaryDataObjects(tlv_data)[0],
                           DCB["ONETIMEWRITE"])
        elif reference == P2_AND:
            ef.writerecord(num_id, reference,
                           decodeOffsetDataObjects(tlv_data)[0],
                           decodeDiscretionaryDataObjects(tlv_data)[0],
                           DCB["WRITEAND"])
        elif reference == P2_OR:
            ef.writerecord(num_id, reference,
                           decodeOffsetDataObjects(tlv_data)[0],
                           decodeDiscretionaryDataObjects(tlv_data)[0],
                           DCB["WRITEOR"])
        else:
            # reference == P2_XOR:
            ef.writerecord(num_id, reference,
                           decodeOffsetDataObjects(tlv_data)[0],
                           decodeDiscretionaryDataObjects(tlv_data)[0],
                           DCB["PROPRIETARY"])

        return SW["NORMAL"], b""

    def appendRecord(self, p1, p2, data):
        """
        Function for instruction 0xe2. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        if p1 != 0 or (p2 & REF["REFERENCE_CONTROL_RECORD"]) != 0:
            raise SwError(SW["ERR_INCORRECTP1P2"])
        ef, num_id, reference = self.recordHandlingDecode(p1, p2)
        sw = ef.appendrecord(data)

        return SW["NORMAL"], b""

    def eraseRecord(self, p1, p2, data):
        """
        Function for instruction 0x0c. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        ef, num_id, reference = self.recordHandlingDecode(p1, p2)
        if reference not in [
                REF["NUMBER"],
                REF["NUMBER_TO_LAST"]]:
            # RFU
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        ef.eraserecord(num_id, reference)

        return SW["NORMAL"], b""

    def dataObjectHandlingDecodePlain(self, p1, p2, data):
        """
        Decodes 'p1', 'p2' and 'data' from a data object handling command (i.
        e. get/put data) with *even* instruction code.

        :returns: the specified file, True if the following list regards
            SIMPLE-TLV data objects False otherwise and a list of
            (tag, length, value)-tuples.
        """
        if self.current is None:
            raise SwError(SW["ERR_NOCURRENTEF"])
        file = self.current

        if ((p1 == 0 and 0x40 <= p2 and p2 <= 0xfe) or
                (0x40 <= p1 and p2 != 0 and p2 != 0xff)):
            # If bit 1 of INS is set to 0 and P1 to '00', then P2 from '40' to
            # 'FE' shall be a BER-TLV tag on a single byte. OR
            # If bit 1 of INS is set to 0 and if P1-P2 lies from '4000' to
            # 'FFFF', then they shall be a BER-TLV tag on two bytes.
            tlv_data = [((p1 << 8) + p2, len(data), data)]
            isSimpleTlv = False
        elif p1 == 0x02 and 0x01 <= p2 and p2 <= 0xfe:
            # If bit 1 of INS is set to 0 and P1 to '02', then P2 from '01' to
            # 'FE' shall be a SIMPLE-TLV tag.
            tlv_data = [(p2, len(data), data)]
            isSimpleTlv = True
        elif p1 == 0x00 and p2 == 0xff:
            # The value '00FF' is used either for obtaining all the common
            # BER-TLV data objects readable in the context, or for indicating
            # that the command data field is encoded in BER-TLV.
            if data:
                tlv_data = bertlv_unpack(data)
            else:
                tlv_data = []
            isSimpleTlv = False
        elif p1 == 0x02 and p2 == 0xff:
            # The value '02FF' is used either for obtaining all the common
            # SIMPLE-TLV data objects readable in the context or for indicating
            # that the command data field is encoded in SIMPLE-TLV.
            if data:
                tlv_data = simpletlv_unpack(data)
            else:
                tlv_data = []
            isSimpleTlv = True
        else:
            raise SwError(SW["ERR_NOTSUPPORTED"])

        return file, isSimpleTlv, tlv_data

    def dataObjectHandlingDecodeEncapsulated(self, p1, p2, data):
        """
        Decodes 'p1', 'p2' and 'data' from a data object handling command (i.
        e. get/put data) with *odd* instruction code.

        :returns: the specified file, True if the following list regards
            SIMPLE-TLV data objects False otherwise and a list of
            (tag, length, value)-tuples.
        """
        # If bit 1 of INS is set to 1, then P1-P2 shall identify a file.
        tlv_data = bertlv_unpack(data)
        if p1 == 0 and p2 == 0:
            # P1-P2 set to '0000' identifies the current EF, unless the command
            # data field provides a file reference data object (tag '51', see
            # 5.3.1.2) for identifying a file.
            file = getfile_byrefdataobj(self,
                                        tlv_find_tag(tlv_data,
                                                     TAG["FILE_REFERENCE"])[0])
            if file is None:
                file = self.currentEF()
            if file is None:
                raise SwError(SW["ERR_NOCURRENTEF"])
        elif p1 == 0 and (p2 >> 5) == 0:
            # If the first eleven bits of P1-P2 are set to 0 and if bits 5 to 1
            # of P2 are not all equal and if the card and / or the file
            # supports selection by short EF identifier, then bits 5 to 1 of P2
            # encode a short EF identifier (a number from one to thirty).
            file = self.currentDF().select('shortfid', p2)
        elif p1 == 0x3f and p2 == 0xff:
            # P1-P2 set to '3FFF' identifies the current DF.
            file = self.currentDF()
        else:
            # Otherwise, P1-P2 is a file identifier.
            file = self.currentDF().select('fid', p1 << 8 + p2)

        if file is None:
            raise SwError(SW["ERR_FILENOTFOUND"])
        self.current = file

        return file, tlv_data

    def getDataPlain(self, p1, p2, data):
        """
        Function for instruction 0xca. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        file, isSimpleTlv, tlvlist = self.dataObjectHandlingDecodePlain(p1,
                                                                        p2,
                                                                        data)
        # TODO oversized answers
        if len(tlvlist) > 0:
            return SW["NORMAL"], file.getdata(isSimpleTlv,
                                              [(tlvlist[0][0], 0)])
        else:
            return SW["NORMAL"], file.getdata(isSimpleTlv, [])

    def getDataEncapsulated(self, p1, p2, data):
        """
        Function for instruction 0xcb. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        file, tlv_data = self.dataObjectHandlingDecodeEncapsulated(p1,
                                                                   p2,
                                                                   data)
        # TODO oversized answers
        requestedTL = decodeTagList(tlv_data)
        if requestedTL == []:
            requestedTL = decodeHeaderList(tlv_data)
            if requestedTL == []:
                requestedTL = decodeExtendedHeaderList(tlv_data)

        return SW["NORMAL"], file.getdata(False, requestedTL)

    def putDataPlain(self, p1, p2, data):
        """
        Function for instruction 0xda. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        file, isSimpleTlv, tlvlist = self.dataObjectHandlingDecodePlain(p1,
                                                                        p2,
                                                                        data)
        file.putdata(isSimpleTlv, tlvlist)

        return SW["NORMAL"], b""

    def putDataEncapsulated(self, p1, p2, data):
        """
        Function for instruction 0xdb. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        file, tlvlist = self.dataObjectHandlingDecodeEncapsulated(p1, p2, data)
        file.putdata(False, tlvlist)

        return SW["NORMAL"], b""

    @staticmethod
    def create(p1, p2, data):
        """
        Creates and returns a list of files according to the parameters of a
        create file command.
        """
        def fdb2args(value, args):
            l = len(value)
            if l >= 5:
                # TODO number of records on one or two bytes
                raise SwError(SW["ERR_NOTSUPPORTED"])
            if l >= 3:
                args["maxrecordsize"] = stringtoint(value[2:])
            if l >= 2:
                args["datacoding"] = value[1]
            if l >= 1:
                args["filedescriptor"] = value[0]

        def shortfid2args(value, args):
            s = stringtoint(value)
            if (s & 7) == 0:
                shortfid = s >> 3
                if shortfid != 0:
                    args["shortfid"] = shortfid

        def unknown(tag, value):
            logging.debug("unknown tag 0x%x with %r" % (tag, value))

        fcp_list = tlv_find_tags(bertlv_unpack(data),
                                 [TAG["FILECONTROLINFORMATION"],
                                  TAG["FILECONTROLPARAMETERS"]])
        if not fcp_list:
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        files = []
        args = {"parent": None}
        if p1 != 0:
            args["filedescriptor"] = p1
        if (p2 >> 3) != 0:
            args["shortfid"] = p2 >> 3
        for T, _, tlv_data in fcp_list:
            if (T != TAG["FILECONTROLPARAMETERS"] and
                    T != TAG["FILECONTROLINFORMATION"]):
                raise ValueError
            for tag, _, value in tlv_data:
                if tag == TAG["FILEDISCRIPTORBYTE"]:
                    fdb2args(value, args)
                elif tag == TAG["FILEIDENTIFIER"]:
                    args["fid"] = stringtoint(value)
                elif tag == TAG["DFNAME"]:
                    args["dfname"] = value
                elif tag == TAG["SHORTFID"]:
                    shortfid2args(value, args)
                elif tag == TAG["LIFECYCLESTATUS"]:
                    args["lifecycle"] = stringtoint(value)
                elif tag == TAG["BYTES_EXCLUDINGSTRUCTURE"]:
                    args["data"] = b'\x00' * stringtoint(value)
                elif tag == TAG["BYTES_INCLUDINGSTRUCTURE"]:
                    args["data"] = b'\x00' * stringtoint(value)
                else:
                    unknown(tag, value)
            print(args)
            if (args["filedescriptor"] & FDB["DF"]) == FDB["DF"]:
                # FIXME: data for DF
                if "data" in args:
                    logging.debug('what to do with DF-data %r?' % args["data"])
                    del args["data"]
                file = DF(**args)
            elif ((args["filedescriptor"] & 7) in
                    [FDB["EFSTRUCTURE_NOINFORMATIONGIVEN"],
                        FDB["EFSTRUCTURE_TRANSPARENT"]]):
                file = TransparentStructureEF(**args)
                file.writebinary(decodeOffsetDataObjects(tlv_data),
                                 decodeDiscretionaryDataObjects(tlv_data))
            else:
                file = RecordStructureEF(**args)

            files.append(file)

        return files

    def createFile(self, p1, p2, data):
        """
        Function for instruction 0xe0. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        df = self.currentDF()
        if df is None:
            raise SwError(SW["ERR_NOCURRENTEF"])

        for file in self.create(p1, p2, data):
            file.parent = df
            df.append(file)
            self.current = file
            logging.info("Created %s" % file)

        return SW["NORMAL"], b""

    def deleteFile(self, p1, p2, data):
        """
        Function for instruction 0xe4. Takes the parameter bytes 'p1', 'p2' as
        integers and 'data' as binary string.

        :returns: the status bytes as two byte long integer and the response
            data as binary string.
        """
        file = self._selectFile(p1, p2, data)
        file.parent.content.remove(file)
        # FIXME: free memory of file and remove its content from the security
        #        device
        logging.info("Deleted %s" % file)

        return SW["NORMAL"], b""


class EF(File):
    """Template class for an elementary file."""
    shortfid = make_property("shortfid", "integer with 1<=shortfid<=30."
                                         "Short EF identifier.")
    datacoding = make_property("datacoding", "integer. Data coding byte.")

    def __init__(self, parent, fid, filedescriptor,
                 lifecycle=LCB["ACTIVATED"],
                 simpletlv_data=None, bertlv_data=None,
                 datacoding=DCB["ONETIMEWRITE"], shortfid=0,
                 extra_fci_data=b''):
        """
        The constructor is supposed to be involved creation of a by creation of
        a TransparentStructureEF or RecordStructureEF.

        See File for more.
        """
        # exlcude FIDs for DFs
        if fid == FID["MF"] or datacoding > 0xFF:
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])
        if shortfid:
            if not (1 <= shortfid and shortfid <= 30):
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])
            self.shortfid = shortfid
        File.__init__(self, parent, fid, filedescriptor, lifecycle,
                      simpletlv_data,
                      bertlv_data, extra_fci_data=extra_fci_data)
        self.datacoding = datacoding


class TransparentStructureEF(EF):
    """Class for an elementary file with transparent structure."""
    data = make_property("data", "string (encrypted). The file's data.")

    def __init__(self, parent, fid,
                 filedescriptor=FDB["EFSTRUCTURE_TRANSPARENT"],
                 lifecycle=LCB["ACTIVATED"],
                 simpletlv_data=None, bertlv_data=None,
                 datacoding=DCB["ONETIMEWRITE"], shortfid=0, data="", 
                 extra_fci_data=b''):
        """
        See EF for more.
        """
        EF.__init__(self, parent, fid,
                    filedescriptor, lifecycle,
                    simpletlv_data, bertlv_data,
                    datacoding, shortfid, extra_fci_data)
        self.data = data

    def readbinary(self, offset):
        """Returns the string of decrypted data beginning at 'offset'."""
        data = self.data

        if offset == 0:
            return data
        else:
            if offset + 1 > len(data):
                raise SwError(SW["ERR_OFFSETOUTOFFILE"])

            return data[offset:]

    def writebinary(self, offsets, datalist, datacoding=None):
        """
        Writes pieces of data to the specified offsets honoring the given
        coding byte.

        :param offsets: list of integers. Offsets.
        :param datalist: list of strings. Data pieces.
        :param datacoding: the data coding byte to use for writing
        """
        data = self.data
        if datacoding:
            data = write(data, datalist, offsets, datacoding)
        else:
            data = write(data, datalist, offsets, self.datacoding)
        self.data = data

    def updatebinary(self, offsets, datalist):
        """
        x.updatebinary(offsets, datalist) <==>
        x.writebinary(offsets, datalist, DCB["ONETIMEWRITE"])
        """
        return self.writebinary(offsets, datalist, DCB["ONETIMEWRITE"])

    def erasebinary(self, erasefrom, eraseto):
        """
        Sets (part of) the content of an EF to its logical erased state,
        sequentially starting from 'erasefrom' ending at 'eraseto'.
        """
        data = self.data
        if erasefrom is None:
            erasefrom = 0
        if eraseto is None:
            eraseto = len(data)

        if erasefrom > len(data):
            raise SwError(SW["ERR_OFFSETOUTOFFILE"])
        if erasefrom > eraseto:
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        data = data[0:erasefrom] + data[eraseto:len(data)]
        self.data = data


class Record(object):
    data = make_property("data", "string. The record's data.")
    identifier = make_property("identifier", "integer with 1 <= identifier <="
                                             " 0xfe. The record's identifier.")
    """Class for a Record of an elementary of record structure"""
    def __init__(self, identifier=None, data=""):
        """
        The constructor is supposed to be involved by EF.appendrecord.
        """
        if identifier:
            if identifier < 0x01 or identifier > 0xFE:
                raise SwError(SW["ERR_INCORRECTPARAMETERS"])
            self.identifier = identifier
        self.data = data

    def __str__(self):
        """Returns a string of the object using an prettyprint_anything."""
        return prettyprint_anything("", self)
    __repr__ = __str__


class RecordStructureEF(EF):
    """Class for an elementary file with record structure."""
    records = make_property("records", "list of records (encrypted)")
    maxrecordsize = make_property("maxrecordsize", "integer. maximum length of"
                                                   " a record's data.")
    recordpointer = make_property("recordpointer", "integer. Points to the "
                                                   "current record (i. e. "
                                                   "index of records).")

    def __init__(self, parent, fid, filedescriptor,
                 lifecycle=LCB["ACTIVATED"],
                 simpletlv_data=None,
                 bertlv_data=None, datacoding=DCB["ONETIMEWRITE"], shortfid=0,
                 maxrecordsize=0xffff, records=[]):
        """
        You should specify the appropriate file descriptor byte to specify
        which kind of record structured file you want to create (i. e.
        linear/cyclic or variable/fixed EF). The record pointer is reset.

        :param records: list of Records
        :para, maxrecordsize: integer. maximum length of a record's data.

        See EF for more.
        """
        if not isinstance(records, list):
            raise TypeError("must be a list of Records")
        EF.__init__(self, parent, fid, filedescriptor, lifecycle,
                    simpletlv_data, bertlv_data,
                    datacoding, shortfid)
        for r in records:
            if (len(r.data) > maxrecordsize or (self.hasFixedRecordSize() and
                                                len(r.data) < maxrecordsize)):
                raise SwError(SW["ERR_INCOMPATIBLEWITHFILE"])
        self.records = records
        self.resetRecordPointer()
        self.maxrecordsize = maxrecordsize

    def resetRecordPointer(self):
        """Resets the record pointer."""
        self.recordpointer = -1

    def isCyclic(self):
        """Returns True if the EF is of cyclic structure, False otherwise."""
        attr = self.filedescriptor & 0x07
        if (attr == FDB["EFSTRUCTURE_CYCLIC_NOFURTHERINFO"] or
                attr == FDB["EFSTRUCTURE_CYCLIC_SIMPLETLV"]):
            return True
        else:
            return False

    def hasSimpleTlv(self):
        """Returns True if the EF is of TLV structure, False otherwise."""
        attr = self.filedescriptor & 0x03
        if (attr == FDB["EFSTRUCTURE_LINEAR_FIXED_SIMPLETLV"] or
                attr == FDB["EFSTRUCTURE_LINEAR_VARIABLESIMPLETLV"] or
                attr == FDB["EFSTRUCTURE_LINEAR_VARIABLESIMPLETLV"]):
            return True
        else:
            return False

    def hasFixedRecordSize(self):
        """Returns True if the records are of fixed size, False otherwise."""
        if (self.filedescriptor == FDB["EFSTRUCTURE_LINEAR_FIXED_SIMPLETLV"] or
                self.filedescriptor ==
                FDB["EFSTRUCTURE_LINEAR_FIXED_NOFURTHERINFO"]):
            return True
        else:
            return False

    def __getRecords(self, num_id, reference):
        """
        Returns a list of records.

        :param num_id:    The requested record's number or identifier
        :param reference: Specifies which record to select (usually the last 3
                          bits of 'p1' of a record handling command)
        """
        if (reference >> 2) == 1:
            return self.__getRecordsByNumber(num_id, reference)
        else:
            return self.__getRecordsByIdentifier(num_id, reference)

    def __getRecordsByNumber(self, number, reference):
        """
        Returns a list of records. Is to be involved by __getRecords.

        :param number: The requested record's number
        :param reference: Specifies which record to select (usually the last 3
            bits of 'p1' of a record handling command)
        """
        result = []
        records = self.records

        if number == 0:
            # refer to the current record
            start = self.recordpointer
        else:
            start = number - 1

        if reference == REF["NUMBER"]:
            end = start + 1
        else:
            # Read all records from number up to the last OR
            # Read all records from the last up to number
            end = len(records)

        result = records[start:end]
        if reference == REF["NUMBER_FROM_LAST"]:
            result.reverse()

        if result == []:
            raise SwError(SW["ERR_RECORDNOTFOUND"])

        return result

    def __getRecordsByIdentifier(self, id, reference):
        """
        Returns a list of records and sets the recordpointer to the first
        record, that matched. Is to be involved by __getRecords.

        :param id: The requested record's identifier
        :param reference: Specifies which record to select (usually the last 3
            bits of 'p1' of a record handling command)
        """
        result = []
        records = self.records

        indexes = get_indexes(records, reference, self.recordpointer)
        for i in indexes:
            if (not self.hasSimpleTlv()) or records[i].identifier == id:
                if result == []:
                    self.recordpointer = i
                result.append(records[i])

        if result == []:
            self.resetRecordPointer()
            raise SwError(SW["ERR_RECORDNOTFOUND"])

        return result

    def readrecord(self, offset, num_id, reference):
        """
        Returns a data string from the given 'offset'. 'num_id' and 'reference'
        specify the record (see __getRecords).
        """
        records = self.__getRecords(num_id, reference)
        result = []
        for r in records:
            if offset == 0:
                result.append(r.data)
            else:
                result.append(r.data[offset:])
                if offset > 0 and offset > len(r.data):
                    offset -= len(r.data)
                else:
                    offset = 0

        return result

    def writerecord(self, num_id, reference, offset, data,
                    datacoding=None):
        """
        Writes a data string to the 'offset' of a record using the given data
        coding byte.  'num_id' and 'reference' specify the record (see
        __getRecords).
        """
        if self.isCyclic() and reference == REF["IDENTIFIER_PREVIOUS"]:
            return self.appendrecord(data)

        records = self.__getRecords(num_id, reference)

        if datacoding:
            records[0].data = write(records[0].data, [data], [0], datacoding,
                                    self.maxrecordsize)
        else:
            records[0].data = write(records[0].data, [data], [0],
                                    self.datacoding, self.maxrecordsize)

        if self.hasSimpleTlv():
            # identifier/tag could have changed
            records[0].identifier = simpletlv_unpack(records[0].data)[0][0]

    def updaterecord(self, num_id, reference, offset, data):
        """
        x.updaterecord(num_id, reference, offset, data) <==>
        x.writerecord(num_id, reference, offset, data, DCB["ONETIMEWRITE"])
        """
        return self.writerecord(num_id, reference, offset, data,
                                DCB["ONETIMEWRITE"])

    def appendrecord(self, data):
        """
        Appends a new record to the file, initializing it with the given data
        string. Sets the recordpointer to the newly created record.
        """
        if self.hasSimpleTlv():
            recordidentifier = simpletlv_unpack(data)[0][0]
        else:
            recordidentifier = None

        l = len(data)
        if l > self.maxrecordsize:
            raise SwError(SW["ERR_INCORRECTPARAMETERS"])

        if self.hasFixedRecordSize():
            data = b'\x00'*(self.maxrecordsize) + data

        records = self.records
        if self.isCyclic():
            records.insert(0, Record(recordidentifier, data))
            self.recordpointer = 0
        else:
            records.append(Record(recordidentifier, data))
            self.recordpointer = len(records)-1
        self.records = records

    def eraserecord(self, num_id, reference):
        """
        Removes a record from the file. 'num_id' and 'reference' specify the
        record (see __getRecords).
        """
        records = self.__getRecords(num_id, reference)
        for r in records:
            r.data = b""
            r.identifier = None
        return SW["NORMAL"]
