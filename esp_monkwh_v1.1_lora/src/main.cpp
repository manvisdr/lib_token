#include <Arduino.h>
#include <SPI.h>
#include <lorawan.h>
#include <LoraMessage.h>

LoraMessage loraPayload;
// loraPayload
//     .addUint8(1)
//     .addUint32(22090001)
//     .addUint32(22090002)
//     .addRawFloat(236.034)
//     .addUint8(-1)
//     .addUint8(-1)
//     .addRawFloat(0.874)
//     .addUint8(-1)
//     .addUint8(-1)
//     .addRawFloat(166.403)
//     .addUint8(-1)
//     .addUint8(-1)
//     .addRawFloat(0.844)
//     .addRawFloat(49.930)
//     .addRawFloat(235789)
//     .addRawFloat(574945)
//     .addRawFloat(6746227)
//     .addRawFloat(0);

// LoraMessage message;

void setup()
{
  Serial.begin(115200);

  LoraMessage message;
  // message.addUint32(22090001);
  message.addLatLng(-33.905024, 151.26648);
  Serial.printf("%02X ", message.getBytes());
  Serial.printf("\n%d ", message.getLength());
  memset(&message, '\0', message.getLength());
}

void loop()
{
  // put your main code here, to run repeatedly:
}