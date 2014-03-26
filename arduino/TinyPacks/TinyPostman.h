//  TinyPostman - Copyright (c) 2012 Francisco Castro <http://fran.cc>
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#ifndef TinyPostman_h
#define TinyPostman_h

#include "TinyPacks.h"

#define TPM_MAX_RESOURCES 4
#define TPM_MAX_PATH_LENGTH 16

#define TPM_GET    0x01
#define TPM_POST   0x02
#define TPM_PUT    0x03
#define TPM_DELETE 0x04

#define TPM_201_Created            0x21
#define TPM_202_Deleted            0x22
#define TPM_204_Changed            0x24
#define TPM_205_Content            0x25
#define TPM_400_Bad_Request        0x40
#define TPM_401_Unauthorized       0x41
#define TPM_403_Forbidden          0x43
#define TPM_404_Not_Found          0x44
#define TPM_405_Method_Not_Allowed 0x45
#define TPM_413_Request_Entity_Too_Large     0x4D

#define TPM_RECEIVING 0
#define TPM_SENDING 1

#define crc16(crc_value, byte_value)					\
    crc_value  = (unsigned char)(crc_value >> 8) | (crc_value << 8);	\
    crc_value ^= (byte_value);						\
    crc_value ^= (unsigned char)(crc_value & 0xff) >> 4;		\
    crc_value ^= (crc_value << 8) << 4;				        \
    crc_value ^= ((crc_value & 0xff) << 4) << 1;

class Request {
    public:
        char path[TPM_MAX_PATH_LENGTH];
        PackReader reader;
        PackWriter writer;
};

class Resource {
    public:
        virtual uint8_t get(Request &request) { return TPM_405_Method_Not_Allowed; };
        virtual uint8_t post(Request &request) { return TPM_405_Method_Not_Allowed; };
        virtual uint8_t put(Request &request) { return TPM_405_Method_Not_Allowed; };
        virtual uint8_t del(Request &request) { return TPM_405_Method_Not_Allowed; };
};

class Postman {
    public:
        Request request;
        struct {
            const char *path;
            Resource * resource;
            // bool subpaths;
        } resources[TPM_MAX_RESOURCES];
        uint8_t registered_resources;

        Postman();
        bool registerResource(const char * path, Resource &resource);
        tp_length_t handlePack(uint8_t * buffer, tp_length_t length, tp_length_t max_length);
};


class Framer {
    private:
        bool        state;
        bool         escape;
        uint16_t   crc;
        uint16_t   crc1;
        uint16_t   crc2;
        tp_length_t    length;
        tp_length_t    max_length;
        tp_length_t    index;
        uint8_t  *  buffer;
    public:
        Framer(uint8_t * pack_buffer, tp_length_t pack_max_length);
        bool putReceivedByte(uint8_t value);
        uint8_t getByteToSend();
        tp_length_t getLength() { return length; };
        void setLength(tp_length_t value) { length = value; };
        bool getState() { return state; };
        void setState(bool value) { state = value; index = crc = crc1 = crc2 = 0; };
};

#endif
