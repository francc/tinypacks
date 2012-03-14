#!/usr/bin/python
#
#  TinyPostman - Copyright (c) 2012 Francisco Castro <http://fran.cc>
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

import time
import crc16
import struct
import serial
import tinypacks

TPM_GET    = 0x01
TPM_POST   = 0x02
TPM_PUT    = 0x03
TPM_DELETE = 0x04

TPM_201_Created            = 0x21
TPM_202_Deleted            = 0x22
TPM_204_Changed            = 0x24
TPM_205_Content            = 0x25
TPM_400_Bad_Request        = 0x40
TPM_401_Unauthorized       = 0x41
TPM_403_Forbidden          = 0x43
TPM_404_Not_Found          = 0x44
TPM_405_Method_Not_Allowed = 0x45
TPM_413_Request_Entity_Too_Large     = 0x4D

TPM_RESPONSE_TEXT = {
    0x21: "201 Created",
    0x22: "202 Deleted",
    0x24: "204 Changed",
    0x25: "205 Content",
    0x40: "400 Bad Request",
    0x41: "401 Unauthorized",
    0x43: "403 Forbidden",
    0x44: "404 Not Found",
    0x45: "405 Method Not Allowed",
    0x4D: "413 Request Entity Too Large",
}

class PostmanError(IOError):
    pass
    
class Postman:
    def __init__(self, device, timeout=4):
        self.debug = False
        self.token = 0
        try:
            self.fd = serial.Serial(device, 9600, stopbits=serial.STOPBITS_ONE, parity=serial.PARITY_NONE, timeout=timeout)
        except IOError as err:
            raise PostmanError("Failed to open port: %s" % err)

    def send(self, method, token, path, has_payload=False, payload=None):
        if has_payload:
            frame = "%s%s%s%s" % (tinypacks.pack(method), tinypacks.pack(token), tinypacks.pack(path), tinypacks.pack(payload))
        else:
            frame = "%s%s%s" % (tinypacks.pack(method), tinypacks.pack(token), tinypacks.pack(path))
        frame +=  struct.pack(">H", crc16.crc16str(frame))

        if self.debug:
            print "TX:",
            for c in frame:
                print "%X" % ord(c),
            print "[%i]" % len(frame)

        try:
            self.fd.write("\x7E")
            for byte in frame:
                if byte == "\x7E":
                    self.fd.write("\x7D\x5E")
                elif byte == "\x7D":
                    self.fd.write("\x7D\x5D")
                else:
                    self.fd.write(byte)
            self.fd.write("\x7E")
        except IOError as err:
            raise PostmanError("Frame sending failed: %s" % err)

    def send_null(self):
        try:
            self.fd.write("\x7E")
        except  IOError as err:
            raise PostmanError("Frame sending failed: %s" % err)

    def receive(self):
        frame = ""
        byte = ""
        try:
            while (byte != "\x7E" or frame == ""):
                byte = self.fd.read(1)
                if not byte:
                    raise PostmanError("Frame receiving timed out.")
                if byte == "\x7D":
                    frame += chr(ord(self.fd.read(1))|32)
                elif byte != "\x7E":
                    frame += byte
        except PostmanError:
            raise
        except  IOError as err:
            raise PostmanError("Frame receiving failed: %s" % err)

        if self.debug:
            print "RX:",
            for c in frame:
                print "%X" % ord(c),
            print "[%i]" % len(frame)

        if len(frame) >= 4 and crc16.crc16str(frame[:-2]) == struct.unpack(">H", frame[-2:])[0]:
            frame = frame[:-2]
            response, frame = tinypacks.unpack(frame)
            if frame:
                token, frame = tinypacks.unpack(frame)
                if frame:
                    payload, frame = tinypacks.unpack(frame)
                    return [response, token, payload]
                else:
                    return [response, token]
            else:
                return [response]
        else:
            raise PostmanError("Frame receiving failed, bad CRC %s != %s " % (hex(crc16.crc16str(frame[:-2])), hex(struct.unpack(">H", frame[-2:])[0])))

    def get(self, path, query=None):
        self.send(TPM_GET, self.token, path, query is not None, query)
        response = self.receive()
        if len(response) > 1 and response[1] != self.token: 
            raise PostmanError("Response token does not match request token.")
        self.token = (self.token + 1) & 0x7F
        return response[0:1] + response[2:]

    def put(self, path, data=None):
        self.send(TPM_PUT, self.token, path, data is not None, data)
        response = self.receive()
        if len(response) > 1 and response[1] != self.token: 
            raise PostmanError("Response token does not match request token.")
        self.token = (self.token + 1) & 0x7F
        return response[0:1] + response[2:]

    def post(self, path, data=None):
        self.send(TPM_POST, self.token, path, data is not None, data)
        response = self.receive()
        if len(response) > 1 and response[1] != self.token: 
            raise PostmanError("Response token does not match request token.")
        self.token = (self.token + 1) & 0x7F
        return response[0:1] + response[2:]

    def delete(self, path, query=None):
        self.send(TPM_DELETE, self.token, path, query is not None, query)
        response = self.receive()
        if len(response) > 1 and response[1] != self.token: 
            raise PostmanError("Response token does not match request token.")
        self.token = (self.token + 1) & 0x7F
        return response[0:1] + response[2:]
