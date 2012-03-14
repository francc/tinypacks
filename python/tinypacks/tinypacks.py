#!/usr/bin/python
#
#  TinyPacks - Copyright (c) 2012 Francisco Castro <http://fran.cc>
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.

import struct

TP_NONE     = 0x00
TP_BOOLEAN  = 0x20
TP_INTEGER  = 0x40
TP_REAL     = 0x60
TP_STRING   = 0x80
TP_BYTES    = 0xA0
TP_LIST     = 0xC0
TP_MAP     = 0xE0

TP_SMALL_SIZE_MASK = 0x1F
TP_SMALL_SIZE_MAX = 0x1E
TP_EXTENDED_SIZE_16 = 0x1F
TP_EXTENDED_SIZE_32 = 0xFFFF

TP_TYPE_MASK    = 0b11100000
TP_FAMILY_MASK  = 0b11000000

TP_NUMBER       = 0b01000000
TP_BLOCK        = 0b10000000
TP_CONTAINER    = 0b11000000


def bit_len(n):
    s = bin(n)       # binary representation:  bin(-37) --> '-0b100101'
    s = s.lstrip('-0b') # remove leading zeros and minus sign
    return len(s)       # len('100101') --> 6

def pack(obj, use_double=False):
    if obj is None:
        return chr(TP_NONE)
    elif isinstance(obj, bool):
        return chr(TP_BOOLEAN|1) + '\x01' if obj else chr(TP_BOOLEAN)
    elif isinstance(obj, (int, long)):
        bit_length = bit_len(obj)
        if bit_length == 0:
            return chr(TP_INTEGER)
        elif bit_length <= 7:
            return struct.pack(">Bb", TP_INTEGER|1, obj)
        elif bit_length <= 15:
            return struct.pack(">Bh", TP_INTEGER|2, obj)
        elif bit_length <= 31:
            return struct.pack(">Bl", TP_INTEGER|4, obj)
        elif bit_length <= 63:
            return struct.pack(">Bq", TP_INTEGER|8, obj)
        else:
            raise ValueError("Integer number too big")
    elif isinstance(obj, float):
        if obj == 0:
            return chr(TP_REAL)
        else:
            if use_double:
                return struct.pack(">Bd", TP_REAL|8, obj)
            else:
                return struct.pack(">Bf", TP_REAL|4, obj)
    elif isinstance(obj, (str, unicode)):
        content = obj.encode('utf_8') if isinstance(obj, unicode) else obj
        byte_length = len(content)
        if byte_length <= TP_SMALL_SIZE_MAX:
            return struct.pack(">B%is" % byte_length, TP_STRING | byte_length, content)
        elif byte_length < 0xFFFF:
            return struct.pack(">BH%is" % byte_length, TP_STRING|TP_EXTENDED_SIZE_16, byte_length, content)
        elif byte_length < 0xFFFFFFFF:
            return struct.pack(">BHL%is" % byte_length, TP_STRING|TP_EXTENDED_SIZE_16, TP_EXTENDED_SIZE_32, byte_length, content)
        else:
            raise ValueError("String too long")
    elif isinstance(obj, bytearray):
        byte_length = len(obj)
        if byte_length <= TP_SMALL_SIZE_MAX:
            return struct.pack(">B%is" % byte_length, TP_BYTES | byte_length, str(obj))
        elif byte_length < 0xFFFF:
            return struct.pack(">BH%is" % byte_length, TP_BYTES|TP_EXTENDED_SIZE_16, byte_length, obj)
        elif byte_length < 0xFFFFFFFF:
            return struct.pack(">BHL%is" % byte_length, TP_BYTES|TP_EXTENDED_SIZE_16, TP_EXTENDED_SIZE_32, byte_length, obj)
        else:
            raise ValueError("Bytearray too long")
    elif isinstance(obj, (list, tuple)):
        content = ''.join([pack(value) for value in obj])
        byte_length = len(content)
        if byte_length <= TP_SMALL_SIZE_MAX:
            return struct.pack(">B%is" % byte_length, TP_LIST | byte_length, content)
        elif byte_length < 0xFFFF:
            return struct.pack(">BH%is" % byte_length, TP_LIST|TP_EXTENDED_SIZE_16, byte_length, content)
        elif byte_length < 0xFFFFFFFF:
            return struct.pack(">BHL%is" % byte_length, TP_LIST|TP_EXTENDED_SIZE_16, TP_EXTENDED_SIZE_32, byte_length, content)
        else:
            raise ValueError("List too long")
    elif isinstance(obj, dict):
        elements = []
        for item in obj.items():
            elements.append(pack(item[0]))
            elements.append(pack(item[1]))
        content = ''.join(elements)
        byte_length = len(content)
        if byte_length <= TP_SMALL_SIZE_MAX:
            return struct.pack(">B%is" % byte_length, TP_MAP | byte_length, content)
        elif byte_length < 0xFFFF:
            return struct.pack(">BH%is" % byte_length, TP_MAP|TP_EXTENDED_SIZE_16, byte_length, content)
        elif byte_length < 0xFFFFFFFF:
            return struct.pack(">BHL%is" % byte_length, TP_MAP|TP_EXTENDED_SIZE_16, TP_EXTENDED_SIZE_32, byte_length, content)
        else:
            raise ValueError("Dict too long")
    else:
        raise ValueError("Unknown type")
        

def unpack(bytes):
    if len(bytes) == 0:
        ValueError("Cannot unpack an empty pack")
    content_type = ord(bytes[0]) & TP_TYPE_MASK    
    content_length = ord(bytes[0]) & TP_SMALL_SIZE_MASK
    element_length = content_length + 1
    if content_length != TP_EXTENDED_SIZE_16:
        content_raw = bytes[1 : element_length]
    else:
        content_length = struct.unpack(">BH", bytes[0:3])[1]
        element_length = content_length + 3
        if content_length != TP_EXTENDED_SIZE_32:
            content_raw = bytes[3 : element_length]
        else:
            content_length = struct.unpack(">BHL", bytes)[2]
            element_length = content_length + 7
            content_raw = bytes[7 : element_length]
    
    if content_type == TP_NONE:
        obj = None
    elif content_type == TP_BOOLEAN:
        obj = True if (content_length and ord(content_raw[0])) else False 
    elif content_type == TP_INTEGER:
        if content_length == 0:
            obj = 0
        elif content_length == 1:
            obj = struct.unpack(">b", content_raw)[0]
        elif content_length == 2:
            obj = struct.unpack(">h", content_raw)[0]
        elif content_length == 4:
            obj = struct.unpack(">l", content_raw)[0]
        elif content_length == 8:
            obj = struct.unpack(">q", content_raw)[0]
        else:
            raise ValueError("Integer number too big")
    elif content_type == TP_REAL:
        if content_length == 0:
            obj = 0.0
        elif content_length == 4:
            obj = struct.unpack(">f", content_raw)[0]
        elif content_length == 8:
            obj = struct.unpack(">d", content_raw)[0]
        else:
            raise ValueError("Real number too big")
    elif content_type == TP_STRING:
        obj = content_raw.decode("utf8")
    elif content_type == TP_BYTES:
        obj = content_raw
    elif content_type == TP_LIST:
        obj = []
        while(content_raw):
            item, content_raw = unpack(content_raw)
            obj.append(item)
    elif content_type == TP_MAP:
        obj = {}
        while(content_raw):
            key, content_raw = unpack(content_raw)
            value, content_raw = unpack(content_raw)
            obj[key] = value
    
    return (obj, bytes[element_length:])
    