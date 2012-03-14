TinyPostman
===========

TinyPostman is a REST-like protocol for reading and writing data resources
from microcontrollers via a serial port. It is based on the TinyPacks data
serialization format and supports the REST methods GET, PUT, POST and DELETE.

The current C++ implementation for Arduino and Atmel AVR microcontrollers uses
about 1.5 KB of Flash and 48 bytes of RAM.

Frame format
------------

The TinyPostman requests and responses are encoded as a sequence of TinyPacks
values as follow:

**Request**

    < method:Integer >  < token:Integer >  < path:String >  [ payload:* ]

**Response**

    < code:Integer >  < token:Integer >  [ payload:* ]

**References**

    method:  An integer value representing one of the following methods:

                1   Get
                2   Post
                3   Put
                4   Delete

    token:   An integer value used as ID for the request and treated as
             opaque value when processing the request.

    path:    The path of the addressed resource encoded as an UTF-8 string.
    
    code:    An integer value representing one of following response codes:
    
                0x21    201 Created
                0x22    202 Deleted
                0x24    204 Changed
                0x25    205 Content
                0x40    400 Bad Request
                0x41    401 Unauthorized
                0x43    403 Forbidden
                0x44    404 Not Found
                0x45    405 Method Not Allowed
                0x4D    413 Request Entity Too Large
                
    payload: An optional payload encoded as one or more TinyPacks values.
    


Code examples
-------------

###Python

    from tinypostman import *

    pm = tinypostman.Postman("/dev/ttyUSB0")

    response = pm.get("")
    if response[0] == TPM_205_Content:
        resources = response[1]
        for resource in resources:
            print(resource + ":")
            response = pm.get(resource)
            if response[0] == TPM_205_Content:
                print(repr(response[1]))
            else:
                print("Cannot get the %s resource." % resource)
    else:
        print("Cannot get the resource index.")


###Arduino

    #include <TinyPostman.h>

    #define   MAX_PACKET_LENGTH 128 
    uint8_t   pack_buffer[MAX_PACKET_LENGTH];

    Postman   postman;
    Framer    framer(pack_buffer, MAX_PACKET_LENGTH);

    class LED : public Resource {
      uint8_t  led_pin;
    public:
      LED() { led_pin = 13; pinMode(led_pin, OUTPUT); };
      
      uint8_t get(Request &request) {
        request.writer.openMap();
        request.writer.putString("pin");
        request.writer.putInteger(led_pin);
        request.writer.putString("state");
        request.writer.putBoolean(digitalRead(led_pin));
        request.writer.close();
        return TPM_205_Content;
      }
      
      uint8_t put(Request &request) {
        request.reader.next();
        if(request.reader.openMap()) {
          while(request.reader.next()) {
            if     (request.reader.match("pin"))     { led_pin = request.reader.getInteger(); pinMode(led_pin, OUTPUT); }
            else if(request.reader.match("state"))   digitalWrite(led_pin, request.reader.getBoolean());
            else request.reader.next();        
          }
          request.reader.close();
          return TPM_204_Changed;
        }
        else
          return TPM_400_Bad_Request;
      }
    } led_resource;


    void setup()
    {
      postman.registerResource("led", led_resource);
      Serial.begin(9600);
    }

    void loop()
    {
      if(framer.getState() == TPM_SENDING)
        Serial.write(framer.getByteToSend());
      while(framer.getState() == TPM_RECEIVING && Serial.available() > 0) {
        if(framer.putReceivedByte(Serial.read()) && framer.getLength()) {
          framer.setLength(postman.handlePack(pack_buffer, framer.getLength(), MAX_PACKET_LENGTH));
          framer.setState(TPM_SENDING);
          break;
        }
      }
    }
