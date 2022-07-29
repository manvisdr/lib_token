#include <Arduino.h>
#include <SoftwareSerial.h>

const byte rxPin = 2;
const byte txPin = 3;

// Set up a new SoftwareSerial object
SoftwareSerial altser(rxPin, txPin);

#define STX 0x02
#define ETX 0x03
#define CAN 0x06

// edmi cmd line test 1B 02 03
unsigned char cmdtest[5] = {0x02, 0x06, 0x06, 0xA4, 0x03};

unsigned char logonReg[19] = {0x02, 0x4C, 0x45, 0x44, 0x4D, 0x49, 0x2C, 0x49, 0x4D, 0x44, 0x45, 0x49, 0x4D, 0x44, 0x45, 0x00, 0xD9, 0x69, 0x03};

// get serial number
// send to meter 		:02 52 F0 10 42 EE 45 03
unsigned char serialNum[18] = {0x02, 0x52, 0xF0, 0x10, 0x42, 0x32, 0x31, 0x37, 0x30, 0x37, 0x32, 0x34, 0x37, 0x30, 0x00, 0x06, 0xA3, 0x03};

// logoff 				02 58 BD 9F 03  0x02,0x58,0xBD,0x9F,0x03
unsigned char logoff[5] = {0x02, 0x06, 0x06, 0xA4, 0x03};

// logoff 				02 58 BD 9F 03  0x02,0x58,0xBD,0x9F,0x03
unsigned char ack[5] = {0x02, 0x06, 0x06, 0xA4, 0x03};

// kwh ch
// send		02 52 1E 01 ED 9B 03  0x02,0x52,0x1E,0x01,0xED,0x9B,0x03
unsigned char kwhreg[11] = {0x02, 0x52, 0x1E, 0x01, 0x4B, 0x9B, 0x1B, 0x08, 0x93, 0x6E, 0x03};

int serIn;
bool flag = false;
char myData[20];

void setup()
{
  Serial.begin(9600);
  altser.begin(9600); // to AltSoftSerial RX
}

void loop()
{
  /* ----------- PARSING STX AND ETX -------------------------------------------
  byte n = Serial.available(); // check that a data item has arrived from sender
  if (n != 0)
  {
    if (flag != true)
    {
      byte x = Serial.read();
      if (x == 0x02)
      {
        flag = true;
      }
    }
    else
    {
      byte m = Serial.readBytesUntil(0x03, myData, 20); // 0x03 is not saved
      myData[m] = '\0';                                 // insert null-charcater;
      Serial.println(myData);                           // shows received string
                                                        //-- add codes to recompute CHKSUM and check validity of data
                                                        //-- add codes to process received string
      memset(myData, 0x00, 20);                         // array is reset to 0s.
      flag = false;
    }
  }
  */

  byte n = altser.available(); // check that a data item has arrived from sender
  if (n != 0)
  {
    if (flag != true)
    {
      byte x = altser.read();
      if (x == 0x02)
      {
        flag = true;
      }
    }
    else
    {
      byte data = altser.read();
      // Serial.println(data);
      switch (data)
      {
      // case 0x3:
      //   altser.write(ack, sizeof(ack));
      //   Serial.write(ack, sizeof(ack));
      //   flag = false;
      case 0x6:
        altser.write(logoff, sizeof(logoff));
        Serial.write(logoff, sizeof(logoff));
        flag = false;
        break;
      case 0x52:
        altser.write(kwhreg, sizeof(kwhreg));
        Serial.write(kwhreg, sizeof(kwhreg));
        flag = false;
        break;
      case 0x4C:
        altser.write(logoff, sizeof(logoff));
        Serial.write(logoff, sizeof(logoff));
        flag = false;
        break;
      case 0x58:
        altser.write(logoff, sizeof(logoff));
        Serial.write(logoff, sizeof(logoff));
        flag = false;
        break;

      default:
        altser.write(ack, sizeof(ack));
        Serial.write(ack, sizeof(ack));
        flag = false;
        break;
      }
    }
  }
}