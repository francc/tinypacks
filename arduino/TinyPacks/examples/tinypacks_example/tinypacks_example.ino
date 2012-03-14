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