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

from virtualsmartcard.utils import stringtoint, inttostring

TAG = {}
TAG["FILECONTROLPARAMETERS"] = 0x62
TAG["FILEMANAGEMENTDATA"] = 0x64
TAG["FILECONTROLINFORMATION"] = 0x6F
TAG["BYTES_EXCLUDINGSTRUCTURE"] = 0x80
TAG["BYTES_INCLUDINGSTRUCTURE"] = 0x81
TAG["FILEDISCRIPTORBYTE"] = 0x82
TAG["FILEIDENTIFIER"] = 0x83
TAG["DFNAME"] = 0x84
TAG["PROPRIETARY_NOTBERTLV"] = 0x85
TAG["PROPRIETARY_SECURITY"] = 0x86
TAG["FIDEF_CONTAININGFCI"] = 0x87
TAG["SHORTFID"] = 0x88
TAG["LIFECYCLESTATUS"] = 0x8A
TAG["SA_EXPANDEDFORMAT"] = 0x8B
TAG["SA_COMPACTFORMAT"] = 0x8C
TAG["FIDEF_CONTAININGSET"] = 0x8D
TAG["CHANNELSECURITY"] = 0x8E
TAG["SA_DATAOBJECTS"] = 0xA0
TAG["PROPRIETARY_SECURITYTEMP"] = 0xA1
TAG["PROPRIETARY_BERTLV"] = 0xA5
TAG["SA_EXPANDEDFORMAT_TEMP"] = 0xAB
TAG["CRYPTIDENTIFIER_TEMP"] = 0xAC
TAG["DISCRETIONARY_DATA"] = 0x53
TAG["DISCRETIONARY_TEMPLATE"] = 0x73
TAG["OFFSET_DATA"] = 0x54
TAG["TAG_LIST"] = 0x5C
TAG["HEADER_LIST"] = 0x5D
TAG["EXTENDED_HEADER_LIST"] = 0x4D


def tlv_unpack(data):
    ber_class = (data[0] & 0xC0) >> 6
    # 0 = primitive, 0x20 = constructed
    constructed = (data[0] & 0x20) != 0
    tag = data[0]
    data = data[1:]
    if (tag & 0x1F) == 0x1F:
        tag = (tag << 8) | data[0]
        while data[0] & 0x80 == 0x80:
            data = data[1:]
            tag = (tag << 8) | data[0]
        data = data[1:]

    length = data[0]
    if length < 0x80:
        data = data[1:]
    elif length & 0x80 == 0x80:
        length_ = 0
        data = data[1:]
        for i in range(0, length & 0x7F):
            length_ = length_ * 256 + data[0]
            data = data[1:]
        length = length_

    value = b"".join(inttostring(i) for i in data[:length])
    rest = data[length:]

    return ber_class, constructed, tag, length, value, rest


def tlv_find_tags(tlv_data, tags, num_results=None):
    """Find (and return) all instances of tags in the given tlv structure (as
    returned by unpack).  If num_results is specified then at most that many
    results will be returned."""

    results = []

    def find_recursive(tlv_data):
        for d in tlv_data:
            t, l, v = d[:3]
            if t in tags:
                results.append(d)
            else:
                if isinstance(v, bytes) and not isinstance(v[0], int):
                    find_recursive(v)

            if num_results is not None and len(results) >= num_results:
                return

    find_recursive(tlv_data)

    return results


def tlv_find_tag(tlv_data, tag, num_results=None):
    """Find (and return) all instances of tag in the given tlv structure (as
    returned by unpack).
    If num_results is specified then at most that many results will be
    returned."""

    return tlv_find_tags(tlv_data, [tag], num_results)


def pack(tlv_data, recalculate_length=False):
    result = b""

    for data in tlv_data:
        tag, length, value = data[:3]
        if tag in (0xff, 0x00):
            result = result + inttostring(tag)
            continue

        if not isinstance(value, bytes):
            value = pack(value, recalculate_length)

        if recalculate_length:
            length = len(value)

        t = b""
        while tag > 0:
            t = inttostring(tag & 0xff) + t
            tag = tag >> 8

        if length < 0x7F:
            l = inttostring(length)
        else:
            l = b""
            while length > 0:
                l = inttostring(length & 0xff) + l
                length = length >> 8
            assert len(l) < 0x7f
            l = inttostring(0x80 | len(l)) + l

        result = result + t
        result = result + l
        result = result + value

    return b"".join(result)


def bertlv_pack(data):
    """Packs a bertlv list of 3-tuples (tag, length, newvalue) into a string"""
    return pack(data)


def unpack(data, with_marks=None, offset=0, include_filler=False):
    result = []
    if isinstance(data, str):
        data = list(map(ord, data))
    while len(data) > 0:
        if data[0] in (0x00, 0xFF):
            if include_filler:
                if with_marks is None:
                    result.append((data[0], None, None))
                else:
                    result.append((data[0], None, None, ()))
            data = data[1:]
            offset = offset + 1
            continue

        l = len(data)
        ber_class, constructed, tag, length, value, data = tlv_unpack(data)
        stop = offset + (l - len(data))
        start = stop - length

        if with_marks is not None:
            marks = []
            for type, mark_start, mark_stop in with_marks:
                if (mark_start, mark_stop) == (start, stop):
                    marks.append(type)
            marks = (marks, )
        else:
            marks = ()

        if not constructed:
            result.append((tag, length, value) + marks)
        else:
            result.append((tag, length,
                           unpack(value, with_marks, offset=start)) + marks)

        offset = stop

    return result


def bertlv_unpack(data):
    """Unpacks a bertlv coded string into a list of 3-tuples (tag, length,
    newvalue)."""
    return unpack(data)


def simpletlv_pack(tlv_data, recalculate_length=False):
    result = b""

    for tag, length, value in tlv_data:
        if tag >= 0xff or tag <= 0x00:
            # invalid
            continue

        if recalculate_length:
            length = len(value)
        if length > 0xffff or length < 0:
            # invalid
            continue

        if length < 0xff:
            result += inttostring(tag) + inttostring(length) + value
        else:
            result += inttostring(tag) + inttostring(0xff) + inttostring(length >> 8) + \
                      inttostring(length & 0xff) + value

    return result


def simpletlv_unpack(data):
    """Unpacks a simpletlv coded string into a list of 3-tuples (tag, length,
    newvalue)."""
    result = []
    if isinstance(data, str):
        data = list(map(ord, data))
    rest = data
    while rest:
        tag = rest[0]
        if tag == 0 or tag == 0xff:
            raise ValueError

        length = rest[1]
        if length == 0xff:
            length = (rest[2] << 8) + rest[3]
            newvalue = rest[4:4+length]
            rest = rest[4+length:]
        else:
            newvalue = rest[2:2+length]
            rest = rest[2+length:]
        result.append((tag, length, newvalue))

    return result


def decodeDiscretionaryDataObjects(tlv_data):
    datalist = []
    tlv_tags = (tlv_find_tags(tlv_data, [TAG["DISCRETIONARY_DATA"],
                                         TAG["DISCRETIONARY_TEMPLATE"]]))
    for (tag, length, newvalue) in tlv_tags:
        datalist.append(newvalue)
    return datalist


def decodeOffsetDataObjects(tlv_data):
    offsets = []
    for (tag, length, newvalue) in tlv_find_tag(tlv_data,
                                                TAG["OFFSET_DATA"]):
        offsets.append(stringtoint(newvalue))
    return offsets


def decodeTagList(tlv_data):
    taglist = []
    for (t, l, data) in tlv_find_tag(tlv_data, TAG["TAG_LIST"]):
        while data:
            tag = data[0]
            data = data[1:]
            if (tag & 0x1F) == 0x1F:
                tag = (tag << 8) | data[0]
                while data[0] & 0x80 == 0x80:
                    data = data[1:]
                    tag = (tag << 8) | data[0]
                data = data[1:]
            taglist.append((tag, 0))
    return taglist


def decodeHeaderList(tlv_data):
    headerlist = []
    for (t, l, data) in tlv_find_tag(tlv_data, TAG["HEADER_LIST"]):
        while data:
            tag = data[0]
            data = data[1:]
            if (tag & 0x1F) == 0x1F:
                tag = (tag << 8) | data[0]
                while data[0] & 0x80 == 0x80:
                    data = data[1:]
                    tag = (tag << 8) | data[0]
                data = data[1:]

                length = data[0]
                if length < 0x80:
                    data = data[1:]
                elif length & 0x80 == 0x80:
                    length_ = 0
                    data = data[1:]
                    for i in range(0, length & 0x7F):
                        length_ = length_ * 256 + data[0]
                        data = data[1:]
                    length = length_

            headerlist.append((tag, length))
    return headerlist


def decodeExtendedHeaderList(tlv_data):
    # TODO
    return []


def encodebertlvDatalist(tag, datalist):
    tlvlist = []
    for data in datalist:
        tlvlist.append((tag, len(data), data))
    return bertlv_pack(tlvlist)


def encodeDiscretionaryDataObjects(datalist):
    return encodebertlvDatalist(TAG["DISCRETIONARY_DATA"], datalist)


def encodeDataOffsetObjects(datalist):
    return encodebertlvDatalist(TAG["OFFSET_DATA"], datalist)
