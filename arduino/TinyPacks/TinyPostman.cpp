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


#include "TinyPostman.h"

// Postman

Postman::Postman()
{
    registered_resources = 0;
}

bool Postman::registerResource(const char * path, Resource &resource)
{
    if(registered_resources < TPM_MAX_RESOURCES) {
        resources[registered_resources].path = path;
        resources[registered_resources].resource = &resource;
        registered_resources += 1;
        return true;
    }
    else
        return false;
}

tp_length_t Postman::handlePack(uint8_t * buffer, tp_length_t length, tp_length_t max_length) 
{
    uint8_t method;
    uint8_t response;
    uint8_t i;
    
    request.reader.setBuffer(buffer, length);
    request.writer.setBuffer(buffer, max_length - 2);
    request.writer.setOffset(2);

    if(!request.reader.next() || !request.reader.isInteger() || !(method = request.reader.getInteger()))
        response = TPM_400_Bad_Request;
    else if(!request.reader.next() || !request.writer.setOffset(request.reader.elementStart() + request.reader.elementLength() - buffer))
        response = TPM_400_Bad_Request;        
    else if(!request.reader.next() || !request.reader.isString() || request.reader.getString(request.path, TPM_MAX_PATH_LENGTH) == TP_INVALID_LENGTH)
        response = TPM_400_Bad_Request;
    else if(method == TPM_GET && !request.path[0]) {
        request.writer.openList();
        for(i = 0; i != registered_resources; i++)
            request.writer.putString(resources[i].path);
        request.writer.close();
        response = TPM_205_Content;
    }
    else {
        for(i = 0; i != registered_resources; i++) {
            if(!strcmp(resources[i].path, request.path)) {
                if(method == TPM_GET) 
                    response = resources[i].resource->get(request);
                else if(method == TPM_POST) 
                    response = resources[i].resource->post(request);
                else if(method == TPM_PUT) 
                    response = resources[i].resource->put(request);
                else if(method == TPM_DELETE)
                    response = resources[i].resource->del(request);
                else
                    response = TPM_400_Bad_Request;
                break;
            }
        }
        if(i == registered_resources)
            response = TPM_404_Not_Found;
    }
    buffer[0] = TP_INTEGER | 1;
    buffer[1] = response;
    return request.writer.getOffset();
}


// Framer

Framer::Framer(uint8_t * pack_buffer, tp_length_t pack_max_length) {
    buffer = pack_buffer;
    max_length = pack_max_length;
    state = TPM_RECEIVING;
    crc = crc1 = crc2 = 0;
    length = 0;
    index = 0;
    escape = false;
}

bool Framer::putReceivedByte(uint8_t value) {
    bool valid_frame = false;
    if(state == TPM_RECEIVING) {
        if(value == 0x7E) {
            valid_frame = index > 2 && crc2 == (buffer[index - 2] << 8 | buffer[index - 1]);
            length = index - 2;
            setState(TPM_RECEIVING);
        }
        else if(value == 0x7D)
            escape = true;
        else {
            value = escape ? value ^ 0x20 : value;
            escape = false;
            crc2 = crc1;
            crc1 = crc;
            crc16(crc, value);
            if(index < max_length)
                buffer[index++] = value;
        }
    }
    return valid_frame;
}

uint8_t Framer::getByteToSend() {
    if(state == TPM_SENDING) {
        if(index == length + 2) {
                setState(TPM_RECEIVING);
                return 0x7E;
        }
        else if(index == length) {
                buffer[index] = crc >> 8;
                buffer[index + 1] = crc & 0xFF;
        }
        
        if(escape) {
            escape = false;
            crc16(crc, buffer[index]);
            return buffer[index++] ^ 0x20;
        }
        else if(buffer[index] == 0x7E || buffer[index] == 0x7D) {
            escape = true;
            return 0x7D;
        }
        else {
            crc16(crc, buffer[index]);
            return buffer[index++];
        }
    }
    else
        return 0x7E;
};
