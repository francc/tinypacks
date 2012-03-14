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

