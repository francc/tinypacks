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

#ifndef TinyPacks_h
#define TinyPacks_h

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef TP_MAX_LEVELS
#define TP_MAX_LEVELS  5
#endif

typedef int32_t tp_integer_t;
#ifdef TP_USE_DOUBLE
typedef double tp_real_t;
#else
typedef float tp_real_t;
#endif

#define TP_SMALL_PACK   0
#define TP_MEDIUM_PACK  1
#define TP_BIG_PACK     2

#define TP_PACK_SIZE    TP_MEDIUM_PACK

#if TP_PACK_SIZE == TP_SMALL_PACK
typedef uint8_t tp_length_t;
#define TP_INVALID_LENGTH 0xFF
#elif TP_PACK_SIZE == TP_MEDIUM_PACK
typedef uint16_t tp_length_t;
#define TP_INVALID_LENGTH 0xFFFF
#elif TP_PACK_SIZE == TP_BIG_PACK
typedef uint32_t tp_length_t;
#define TP_INVALID_LENGTH 0xFFFFFFFF
#else
#error "TP_PACK_SIZE must be defined as TP_SMALL_PACK, TP_MEDIUM_PACK or TP_BIG_PACK."
#endif


#define TP_NONE      0x00
#define TP_BOOLEAN   0x20
#define TP_INTEGER   0x40
#define TP_REAL      0x60
#define TP_STRING    0x80
#define TP_BYTES     0xA0
#define TP_LIST      0xC0
#define TP_MAP       0xE0

#define TP_SMALL_SIZE_MASK   0x1F
#define TP_SMALL_SIZE_MAX    0x1E
#define TP_EXTENDED_SIZE_16  0x1F
#define TP_EXTENDED_SIZE_32  0xFFFF

#define TP_TYPE_MASK    0b11100000
#define TP_FAMILY_MASK  0b11000000

#define TP_NUMBER       0b01000000
#define TP_BLOCK        0b10000000
#define TP_CONTAINER    0b11000000



class PackReader {
    private:
        struct level {
            uint8_t * element_start;
            uint8_t * content_start;
            uint8_t * parent_start;
            tp_length_t element_length;
            tp_length_t content_length;            
            tp_length_t parent_length;
        } levels[TP_MAX_LEVELS];
        struct level * cursor;

    public:
        PackReader() {};
        PackReader(uint8_t * buffer, tp_length_t length);
        void setBuffer(uint8_t * buffer, tp_length_t length);
    
        uint8_t getType() { return (cursor->element_start[0] & TP_TYPE_MASK); };
        bool isNone()        { return getType() == TP_NONE; };
        bool isBoolean()     { return getType() == TP_BOOLEAN; };
        bool isInteger()     { return getType() == TP_INTEGER; };
        bool isReal()        { return getType() == TP_REAL; };
        bool isString()      { return getType() == TP_STRING; };
        bool isBytes()       { return getType() == TP_BYTES; };
        bool isList()        { return getType() == TP_LIST; };
        bool isMap()         { return getType() == TP_MAP; };
        bool isNumber()      { return (cursor->element_start[0] & TP_FAMILY_MASK) == TP_NUMBER; };
        bool isBlock()       { return (cursor->element_start[0] & TP_FAMILY_MASK) == TP_BLOCK; };
        bool isContainer()   { return (cursor->element_start[0] & TP_FAMILY_MASK) == TP_CONTAINER; };

        bool          getBoolean();
        tp_integer_t  getInteger();
        tp_real_t     getReal();
        tp_length_t   getString(char *string, tp_length_t max_length);
        tp_length_t   getBytes(uint8_t *bytes, tp_length_t max_length);

        bool equals(char *string);
        bool match(char *string);
        
        bool next();
        bool hasNext()       { return cursor->element_start + cursor->element_length < cursor->parent_start + cursor->parent_length; };
        
        uint8_t *    elementStart()  { return cursor->element_start; };
        tp_length_t  elementLength() { return cursor->element_length; };
        uint8_t *    contentStart()  { return cursor->content_start; };
        tp_length_t  contentLength() { return cursor->content_length; };
        
        bool open();
        bool openList() { return isList() && open(); };
        bool openMap()  { return isMap() && open(); };
        bool close()    { return cursor != &levels[0] ? cursor--, true : false; };       
};



class PackWriter {
    private:
        uint8_t * buffer_start;
        tp_length_t buffer_length;
    
        uint8_t * cursor;
        uint8_t * container_start[TP_MAX_LEVELS];
        uint8_t level;
    public:
        PackWriter() {};
        PackWriter(uint8_t * buffer, tp_length_t max_length);
        void setBuffer(uint8_t * buffer, tp_length_t max_length);

        bool  put(uint8_t type, tp_length_t length);
        bool  putBoolean(bool value);
        bool  putInteger(tp_integer_t value);
        bool  putReal(tp_real_t value);
        bool  putString(const char *value);
        bool  putBytes(uint8_t *value, tp_length_t length);
    
        bool  open(uint8_t type);
        bool  openList() { return open(TP_LIST); };
        bool  openMap() { return open(TP_MAP); };
        bool  close();
        
        tp_length_t  getOffset() { return cursor - buffer_start; };
        bool  setOffset(tp_length_t offset);
};

#endif