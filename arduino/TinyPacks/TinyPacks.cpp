//  TinyPacks - Copyright (c) 2012 Francisco Castro <http://fran.cc>
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

#include "TinyPacks.h"

PackReader::PackReader(uint8_t * buffer, tp_length_t length)
{
    setBuffer(buffer, length);
}

void PackReader::setBuffer(uint8_t * buffer, tp_length_t length)
{
    cursor = &levels[0];
    cursor->element_start = buffer;
    cursor->element_length = 0;
    cursor->parent_start = buffer;
    cursor->parent_length = length;
}

bool PackReader::next()
{
    if(hasNext()) {
        cursor->element_start +=  cursor->element_length;
        if((cursor->element_start[0] & TP_SMALL_SIZE_MASK) != TP_EXTENDED_SIZE_16) {
            cursor->content_length = cursor->element_start[0] & TP_SMALL_SIZE_MASK;
            cursor->content_start = cursor->element_start + 1;
            cursor->element_length = cursor->content_length + 1;
            return true;
        }
        else {
            #if TP_PACK_SIZE == TP_MEDIUM_PACK ||  TP_PACK_SIZE == TP_BIG_PACK
            cursor->content_length = (tp_length_t)( \
                cursor->element_start[1] << 8 | \
                cursor->element_start[2] << 0 );
            if(cursor->content_length != TP_EXTENDED_SIZE_32) {
                cursor->content_start = cursor->element_start + 3;
                cursor->element_length = cursor->content_length + 3;
                return true;
            }
            else {
                #if TP_PACK_SIZE == TP_BIG_PACK
                cursor->content_length = (tp_length_t)( \
                    cursor->element_start[3] << 24 | \
                    cursor->element_start[4] << 16 | \
                    cursor->element_start[5] <<  8 | \
                    cursor->element_start[6] <<  0 );
                cursor->content_start = cursor->element_start + 7;
                cursor->element_length = cursor->content_length + 7;
                return true;
                #else
                cursor->element_length = 0;
                cursor->parent_length = 0;        // make hasNext returns false
                return false;
                #endif
            }
            #else
            cursor->element_length = 0;
            cursor->parent_length = 0;        // make hasNext returns false
            return false;
            #endif
        }
    }
    else
        return false;
}
   
bool PackReader::open() 
{
    if(isContainer() && cursor->content_length != 0 && cursor < &levels[TP_MAX_LEVELS - 1]) {
        (cursor+1)->element_start = cursor->element_start;
        (cursor+1)->element_length = cursor->content_start - cursor->element_start;
        (cursor+1)->parent_start = cursor->content_start;
        (cursor+1)->parent_length = cursor->content_length;    
        cursor++;
        return true;
    }
    else
        return false;
}

bool PackReader::equals(char *string)
{
    return isString() && cursor->content_length == strlen(string) && \
        strncmp(string, (char *)cursor->content_start, cursor->content_length) == 0;
}

bool PackReader::match(char *string) 
{
    if(!equals(string)) return false;
    next();
    return true;
}

bool PackReader::getBoolean()
{
    if(cursor->content_length == 0 || !isBoolean())
        return false;
    else
        return (bool)cursor->content_start[0];
}

tp_integer_t PackReader::getInteger()
{
    if(isInteger()) {
        if(cursor->content_length == 1)
            return (tp_integer_t)((int8_t) cursor->content_start[0]);
        else if(sizeof(tp_integer_t) >= 2 && cursor->content_length == 2)
            return (tp_integer_t)((int16_t) cursor->content_start[0] << 8 | \
                          (int16_t) cursor->content_start[1] << 0 );                
        else if(sizeof(tp_integer_t) >= 4 && cursor->content_length == 4)
            return (tp_integer_t)((int32_t) cursor->content_start[0] << 24 | \
                          (int32_t) cursor->content_start[1] << 16 | \
                          (int32_t) cursor->content_start[2] <<  8 | \
                          (int32_t) cursor->content_start[3] <<  0 );
        else
            return 0;
    }
    else if(isReal())
        return (tp_integer_t) getReal();
    else
        return 0;
}

tp_real_t PackReader::getReal()
{
    if(isReal()) {
        if(cursor->content_length == 4) {
            uint32_t float_bytes = (uint32_t) cursor->content_start[0] << 24 | \
                                   (uint32_t) cursor->content_start[1] << 16 | \
                                   (uint32_t) cursor->content_start[2] <<  8 | \
                                   (uint32_t) cursor->content_start[3] <<  0 ;
            return * (float *) &float_bytes;
        }
        else if(sizeof(tp_real_t) == 8 && cursor->content_length == 8) {
            uint64_t double_bytes = (uint64_t) cursor->content_start[0] << 56 | \
                                    (uint64_t) cursor->content_start[1] << 48 | \
                                    (uint64_t) cursor->content_start[2] << 40 | \
                                    (uint64_t) cursor->content_start[3] << 32 | \
                                    (uint64_t) cursor->content_start[4] << 24 | \
                                    (uint64_t) cursor->content_start[5] << 16 | \
                                    (uint64_t) cursor->content_start[6] <<  8 | \
                                    (uint64_t) cursor->content_start[7] <<  0 ;
            return * (double *) &double_bytes;
        }
        else
            return 0;
    }
    else if(isInteger())
        return (tp_real_t) getInteger();
    else
        return 0;
}

tp_length_t PackReader::getString(char *string, tp_length_t max_length)
{
    if(cursor->content_length > max_length - 1)
        return TP_INVALID_LENGTH;
    else {
        strncpy(string, (char *)cursor->content_start, cursor->content_length);
        string[cursor->content_length] = 0;
        return cursor->content_length;
    }
}

tp_length_t PackReader::getBytes(uint8_t *bytes, tp_length_t max_length)
{
    if(cursor->content_length > max_length)
        return TP_INVALID_LENGTH;
    else {
        memcpy(bytes, cursor->content_start, cursor->content_length);
        return cursor->content_length;
    }
}

/// Writer

PackWriter::PackWriter(uint8_t * buffer, tp_length_t max_length)
{
    setBuffer(buffer, max_length);
}

void PackWriter::setBuffer(uint8_t * buffer, tp_length_t max_length)
{
    buffer_start = buffer;
    buffer_length = max_length;
    cursor = buffer;
    level = 0;
}

bool PackWriter::put(uint8_t type, tp_length_t length)
{
    if(length <= TP_SMALL_SIZE_MAX) {
        if((cursor - buffer_start + 1 + length) <= buffer_length) {
            cursor[0] = type | length;
            cursor += 1;
            return true;
        }
        else
            return false;
    }
#if TP_PACK_SIZE == TP_MEDIUM_PACK ||  TP_PACK_SIZE == TP_BIG_PACK
    else if(length < 0xFFFF) {
        if((cursor - buffer_start + 3 + length) <= buffer_length) {
            cursor[0] = type | TP_EXTENDED_SIZE_16;
            cursor[1] = (length >> 8) & 0xFF;
            cursor[2] = (length >> 0) & 0xFF;
            cursor += 3;
            return true;
        }
        else
            return false;
    }
#endif
#if TP_PACK_SIZE == TP_BIG_PACK
    else if(length < 0xFFFFFFFF) {
        if((cursor - buffer_start + 7 + length) <= buffer_length) {
            cursor[0] = type | TP_EXTENDED_SIZE_16;
            cursor[1] = TP_EXTENDED_SIZE_32 >> 8;
            cursor[2] = TP_EXTENDED_SIZE_32 & 0xFF;
            cursor[3] = (length >> 24) & 0xFF;
            cursor[4] = (length >> 16) & 0xFF;
            cursor[5] = (length >>  8) & 0xFF;
            cursor[6] = (length >>  0) & 0xFF;
            cursor += 7;
            return true;
        }
        else
            return false;
    }
#endif
    else
        return false;
}

bool PackWriter::putBoolean(bool value)
{
    if(value) {
        if(!put(TP_BOOLEAN, 1))
            return false;
        cursor[0] = true;
        cursor += 1;
        return true;
    }
    else
        return put(TP_BOOLEAN, 0);
}

bool  PackWriter::putInteger(tp_integer_t value)
{
    if(value >= INT8_MIN && value <= INT8_MAX) {
        if(!put(TP_INTEGER, 1))
            return false;
        cursor[0] = value & 0xFF;
        cursor += 1;
        return true;
    }
    else if(value >= INT16_MIN && value <= INT16_MAX) {
        if(!put(TP_INTEGER, 2))
            return false;
        cursor[0] = (value >> 8) & 0xFF;
        cursor[1] = (value >> 0) & 0xFF;
        cursor += 2;
        return true;
    }
    else {
        if(!put(TP_INTEGER, 4))
            return false;
        cursor[0] = (value >> 24) & 0xFF;
        cursor[1] = (value >> 16) & 0xFF;
        cursor[2] = (value >>  8) & 0xFF;
        cursor[3] = (value >>  0) & 0xFF;
        cursor += 4;
        return true;
    }
}

bool  PackWriter::putReal(tp_real_t value)
{
    if(!value)
        return put(TP_REAL, 0);
    else if(sizeof(tp_real_t) == 4) {
        if(put(TP_REAL, 4)) {
            cursor[0] = (*(uint32_t *)(&value) >> 24) & 0xFF;
            cursor[1] = (*(uint32_t *)(&value) >> 16) & 0xFF;
            cursor[2] = (*(uint32_t *)(&value) >>  8) & 0xFF;
            cursor[3] = (*(uint32_t *)(&value) >>  0) & 0xFF;
            cursor += 4;
            return true;
        } else
            return false;
    }
    else if(sizeof(tp_real_t) == 8) {
        if(put(TP_REAL, 8)) {
            cursor[0] = (*(uint64_t *)(&value) >> 56) & 0xFF;
            cursor[1] = (*(uint64_t *)(&value) >> 48) & 0xFF;
            cursor[2] = (*(uint64_t *)(&value) >> 40) & 0xFF;
            cursor[3] = (*(uint64_t *)(&value) >> 32) & 0xFF;
            cursor[4] = (*(uint64_t *)(&value) >> 24) & 0xFF;
            cursor[5] = (*(uint64_t *)(&value) >> 16) & 0xFF;
            cursor[6] = (*(uint64_t *)(&value) >>  8) & 0xFF;
            cursor[7] = (*(uint64_t *)(&value) >>  0) & 0xFF;
            cursor += 8;
            return true;
        }  else
            return false;
    }
    return false;
}

bool  PackWriter::putString(const char *value)
{
    tp_length_t value_length = strlen(value);
    if(!put(TP_STRING, value_length))
        return false;    
     memcpy(cursor, value, value_length);
     cursor += value_length;
     return true;
}
    
bool  PackWriter::putBytes(uint8_t *value, tp_length_t value_length)
{
    if(!put(TP_BYTES, value_length))
        return false;    
     memcpy(cursor, value, value_length);
     cursor += value_length;
     return true;
}

bool PackWriter::open(uint8_t type)
{
#if TP_PACK_SIZE == TP_SMALL_PACK
    if(level < TP_MAX_LEVELS && (cursor - buffer_start + 1) <= buffer_length) {
        cursor[0] = type;
        container_start[level] = cursor;
        cursor += 1;
        level += 1;
        return true;
    }
#elif TP_PACK_SIZE == TP_MEDIUM_PACK
    if(level < TP_MAX_LEVELS && (cursor - buffer_start + 3) <= buffer_length) {
        cursor[0] = type | TP_EXTENDED_SIZE_16;
        container_start[level] = cursor;
        cursor += 3;
        level += 1;
        return true;
    }
#elif TP_PACK_SIZE == TP_BIG_PACK
    if(level < TP_MAX_LEVELS && (cursor - buffer_start + 7) <= buffer_length) {
        cursor[0] = type | TP_EXTENDED_SIZE_16;
        cursor[1] = TP_EXTENDED_SIZE_32 >> 8;
        cursor[2] = TP_EXTENDED_SIZE_32 & 0xFF;
        container_start[level] = cursor;
        cursor += 7;
        level += 1;
        return true;
    }
#else
#error "TP_PACK_SIZE must be defined as TP_SMALL_PACK, TP_MEDIUM_PACK or TP_BIG_PACK."    
#endif
    else
        return false;
}

bool PackWriter::close()
{
    if(level) {
#if TP_PACK_SIZE == TP_SMALL_PACK
        tp_length_t container_length = cursor - container_start[level - 1] - 1;
        if(container_length <= TP_SMALL_SIZE_MAX)
            container_start[level - 1][0] |= container_length;
        else
            return false;
#elif TP_PACK_SIZE == TP_MEDIUM_PACK
        tp_length_t container_length = cursor - container_start[level - 1] - 3;
        if(container_length < 0xFFFF) {
            container_start[level - 1][1] = (container_length >> 8) & 0xFF;
            container_start[level - 1][2] = (container_length >> 0) & 0xFF;
        }
        else
            return false;
#elif TP_PACK_SIZE == TP_BIG_PACK
        tp_length_t container_length = cursor - container_start[level - 1] - 7;
        if(container_length < 0xFFFFFFFF) {
            container_start[level - 1][3] = (container_length >> 24) & 0xFF;
            container_start[level - 1][4] = (container_length >> 16) & 0xFF;
            container_start[level - 1][5] = (container_length >>  8) & 0xFF;
            container_start[level - 1][6] = (container_length >>  0) & 0xFF;
        }
        else
            return false;
#else
#error "TP_PACK_SIZE must be defined as TP_SMALL_PACK, TP_MEDIUM_PACK or TP_BIG_PACK."    
#endif
        level -= 1;
        return true;
    }
    else
        return false;
}

bool PackWriter::setOffset(tp_length_t offset)
{
    if(offset < buffer_length) {
        cursor = buffer_start + offset;
        return true;
    }
    else
        return false;
}
