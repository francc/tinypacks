TinyPacks
=========

TinyPacks is data serialization format for constrained environments like 8-bit
and 16-bit microcontrollers. It was designed to achieve these goals:

 * Easy to traverse and to skip entire nested elements.
 * Small serializated data footprint.
 * Small encoder/decoder memory footprint.
 * In-place parsing using static memory allocation.
 * Easy translation to and from JSON

The current C++ implementation for Arduino and Atmel AVR microcontrollers uses
about 2 KB of Flash and 9 bytes of RAM plus 14 bytes of RAM  per nesting level
in the source data.

Data types
----------

    None        Null value.
    Boolean     Boolean True or False value.
    Integer     Signed integer 8, 16, 32 or 64 bits long.
    Real        Floating point number 32 or 64 bits long.
    String      Variable length string of characters encoded as UTF-8.
    Bytes       Variable length raw sequence of bytes.
    List        Sequence of values.
    Map         Sequence of key-value pairs.


Serialization format
--------------------

###Specification syntax

    |    Alternative
    [ ]  Repetition, zero or more elements
    [n]  Repetition, n elements
    :    Length in bits, element:length
    *    Arithmetic multiplication
    @    Native C type using network byte order
    -    Range
    #    Define parameter, element#name

###Format specification

    document =       element[]

    element =        none | boolean | integer | real | string | bytes | list | map

    none =           0:3   0:5

    boolean =        false | true
    false =          1:3   0:5
    true =           1:3   1:5          1:8

    integer =        integer0 | integer8 | integer16 | integer32 | integer64
    integer0 =       2:3   0:5
    integer8 =       2:3   1:5          @int8_t:8
    integer16 =      2:3   2:5          @int16_t:16
    integer32 =      2:3   4:5          @int32_t:32
    integer64 =      2:3   8:5          @int64_t:64

    real =           real0 | real32 | real64
    real0 =          3:3   0:5
    real32 =         3:3   4:5          @float:32
    real64 =         3:3   8:5          @double:64

    string =         small_string | medium_string | big_string
    small_string =   4:3   0-0x1E#n:5   @char[n]:8*n
    medium_string =  4:3   0x1F:5       0-0xFFFE#n:16             @char[n]:8*n
    big_string =     4:3   0x1F:5       0xFFFF:16                 0-0xFFFFFFFE#n:32         @char[n]:8*n

    bytes =          small_bytes | medium_bytes | big_bytes
    small_bytes =    5:3   0-0x1E#n:5   @uint8_t[n]:8*n
    medium_bytes =   5:3   0x1F:5       0-0xFFFE#n:16             @uint8_t[n]:8*n
    big_bytes =      5:3   0x1F:5       0xFFFF:16                 0-0xFFFFFFFE#n:32         @uint8_t[n]:8*n

    list =           small_list | medium_list | big_list
    small_list =     6:3   0-0x1E#n:5   element[]:8*n
    medium_list =    6:3   0x1F:5       0-0xFFFE#n:16             element[]:8*n
    big_list =       6:3   0x1F:5       0xFFFF:16                 0-0xFFFFFFFE#n:32         element[]:8*n

    map =            small_map | medium_map | big_map
    small_map =      7:3   0-0x1E#n:5   (element element)[]:8*n
    medium_map =     7:3   0x1F:5       0-0xFFFE#n:16             (element element)[]:8*n
    big_map =        7:3   0x1F:5       0xFFFF:16                 0-0xFFFFFFFE#n:32         (element element)[]:8*n


Serialization examples
----------------------

    Python                                      TinyPacks
    ------                                      ---------
    None                                        00
    0                                           40
    123                                         41 7b
    4567                                        42 11 d7
    8.9                                         64 41 0e 66 66
    0.0                                         60
    True                                        21 01
    False                                       20
    "ABC"                                       83 41 42 43
    "hello world!"                              8c 68 65 6c 6c 6f 20 77 6f 72
                                                6c 64 21
    "A string longer than 30 characters."       9f 00 23 41 20 73 74 72 69 6e
                                                67 20 6c 6f 6e 67 65 72 20 74
                                                68 61 6e 20 33 30 20 63 68 61
                                                72 61 63 74 65 72 73 2e
    b"\x01\x02\x03"                             a3 01 02 03
    [1, 2, 3]                                   c6 41 01 41 02 41 03
    [4, True, "fun"]                            c8 41 04 21 01 83 66 75 6e
    {"a": 1, "b": False, "c": "foo"}            ed 81 61 41 01 81 63 83 66 6f
                                                6f 81 62 20
    {"foo": [1, 2], "bar": {True: 3, False: 4}} f5 83 66 6f 6f c4 41 01 41 02
                                                83 62 61 72 e7 20 41 04 21 01
                                                41 03


Code examples
-------------

###Python

    import tinypacks

    data = { "message": "Hello world!", "status": True, "count": 123 }
    print("Data: " +  repr(data))

    packed_data = tinypacks.pack(data)
    print("Packed: " + repr(packed_data))

    (unpacked_data, remaining_data) = tinypacks.unpack(packed_data)
    print("Unpacked: " + repr(unpacked_data))


###Arduino

    #include <TinyPacks.h>

    PackWriter writer;
    PackReader reader;

    #define MAX_PACKED_DATA 256
    unsigned char packed_data[MAX_PACKED_DATA];
    int packed_data_length;

    void setup()
    {
      Serial.begin(9600);
    }

    void loop()
    {
      #define MAX_TEXT_LENGTH 32
      char text[MAX_TEXT_LENGTH] = "";
      bool status = false;
      int  count = 0;

      // Pack
      writer.setBuffer(packed_data, MAX_PACKED_DATA);
      writer.openMap();
      writer.putString("text");
      writer.putString("Hello world!");
      writer.putString("status");
      writer.putBoolean(true);
      writer.putString("count");
      writer.putInteger(123);
      writer.close();
      packed_data_length = writer.getOffset();

      // Unpack
      reader.setBuffer(packed_data, packed_data_length);
      reader.next();
      if(reader.openMap()) {
        while(reader.next()) {
          if     (reader.match("status"))  status = reader.getBoolean();
          else if(reader.match("count"))   count = reader.getInteger();
          else if(reader.match("text"))    reader.getString(text, MAX_TEXT_LENGTH);
          else reader.next();
        }
        reader.close();
      }

      // Print unpacked data
      Serial.println("Map content:");
      Serial.print("  text: ");
      Serial.println(text);
      Serial.print("  status: ");
      Serial.println(status);
      Serial.print("  count: ");
      Serial.println(count);
      Serial.println();

      delay(5000);
    }
