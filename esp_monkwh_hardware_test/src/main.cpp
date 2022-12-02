#include <Arduino.h>
#include <SPI.h>
#include <lorawan.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#define NUL 0x00 // NULL termination character
#define STX 0x02 // START OF TEXT
#define ETX 0x03 // END OF TEXT
#define ACK 0x06 // ACKNOWLEDGE
#define DLE 0x10 // DATA LINE ESCAPE

#define RXpin 16
#define TXpin 17
#define RXgps 21
#define TXgps 22
#define ENGPSReg 33
static const uint32_t GPSBaud = 9600;
static const int INTERCHAR_TIMEOUT = 100; // MAX TIMEOUT EVERY CHAR

// #define ENABLE_ETH
#ifdef ENABLE_ETH
#include <EthernetSPI2.h>
#define csEthOne 4
// ETHERNET CONF
byte macEthOne[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ipEthOne(10, 10, 10, 10); // ethernet 1
EthernetServer serverOne(80);
#endif

#define ENABLE_LORA
#ifdef ENABLE_LORA
#define US_915
const char *devAddr = "00000000";
const char *nwkSKey = "D738F0B8650E63EB3610AFD127D7B1F1";
const char *appSKey = "7566E203896B5DCC170DDE0B602BA9AA";

const sRFM_pins RFM_pins = {
    .CS = 5,
    .RST = 26,
    .DIO0 = 25,
    .DIO1 = 32,
};
#endif

TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXgps, TXgps);

const unsigned long interval = 1000; // 10 s interval to send message
unsigned long previousMillis = 0;    // will store last time message sent
unsigned int counter = 0;            // message counter

char myStr[50];
char outStr[255];
byte recvStatus = 0;
bool MK10Test = false;

void displayGPS()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10)
      Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}

static uint16_t ccitt_16[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108,
    0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210,
    0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B,
    0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401,
    0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE,
    0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6,
    0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D,
    0xC7BC, 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5,
    0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC,
    0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4,
    0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD,
    0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13,
    0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A,
    0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E,
    0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1,
    0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB,
    0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0,
    0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8,
    0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657,
    0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9,
    0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882,
    0x28A3, 0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E,
    0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07,
    0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D,
    0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74,
    0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};

static unsigned short
CalculateCharacterCRC16(unsigned short crc, uint8_t c)
{
  // debus.printf("\n");
  return ((crc << 8) ^ ccitt_16[((crc >> 8) ^ c)]);
}

static uint16_t
CRC16buf(const uint8_t *data, uint16_t len)
{
  uint8_t nTemp = STX; // CRC table index
  uint16_t crc = 0;    // Default value
  while (len--)
  {
    crc = (crc << 8) ^ ccitt_16[((crc >> 8) ^ nTemp)];
    nTemp = *data++;
  }
  crc = (crc << 8) ^ ccitt_16[((crc >> 8) ^ nTemp)];
  return crc;
}

bool checkCRC(uint8_t *buf, uint16_t len)
{
  if (len <= 3)
    return false;

  uint16_t crc = CRC16buf(buf, len - 2); // Compute CRC of data
  return ((uint16_t)buf[len - 2] << 8 | (uint16_t)buf[len - 1]) == crc;
}

bool RX_char(unsigned int timeout, byte *inChar)
{
  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;

  while (currentMillis - previousMillis < timeout)
  {
    if (Serial2.available())
    {
      (*inChar) = (byte)Serial2.read();
      return true;
    }
    currentMillis = millis();
  } // while
  return false;
}

uint8_t RX_message(uint8_t *message, int maxMessageSize, unsigned int timeout)
{
  byte inChar;
  int index = 0;
  bool completeMsg = false;
  bool goodCheksum = false;
  bool dlechar = false;
  static unsigned short crc;

  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;

  while (currentMillis - previousMillis < timeout)
  {
    if (Serial2.available())
    {
      inChar = (byte)Serial2.read();

      if (inChar == STX) // start of message?
      {
        while (RX_char(INTERCHAR_TIMEOUT, &inChar))
        {
          if (inChar != ETX)
          {
            if (inChar == DLE)
              dlechar = true;
            else
            {
              if (dlechar)
                inChar &= 0xBF;
              dlechar = false;
              message[index] = inChar;
              index++;
              if (index == maxMessageSize) // buffer full
                break;
            }
          }
          else // got ETX, next character is the checksum
          {
            if (index > 6)
            {
              if (checkCRC(message, index))
              {
                message[index] = '\0'; // good checksum, null terminate message
                goodCheksum = true;
                completeMsg = true;
              }
            }
            else
            {
              message[index] = '\0';
              completeMsg = true;
            }
          } // next character is the checksum
        }   // while read until ETX or timeout
      }     // inChar == STX
    }       // Serial2.available()
    if (index > 6)
    {
      if (completeMsg and goodCheksum)
        break;
    }
    else
    {
      if (completeMsg)
        break;
    }
    currentMillis = millis();
  } // while
  return index - 3;
}

bool MK10SerialTest()
{
  Serial2.write(STX);
  Serial2.write(ETX);
  uint8_t
      charAck[3];
  RX_message(charAck, 3, 500);
  if (charAck[0] == ACK)
  {
    Serial.println("received ACK -> SERIAL KWH OK");
    return true;
  }
  else
  {
    return false;
    Serial.println("not received -> SERIAL KWH FAILURE");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXpin, TXpin);
  pinMode(ENGPSReg, OUTPUT);
  digitalWrite(ENGPSReg, HIGH);
  gpsSerial.begin(GPSBaud);
  MK10SerialTest();

#ifdef ENABLE_LORA
  lora.init();
  if (!lora.init())
  {
    Serial.println("Lora Not Detected -> LORA FAILURE");
    delay(5000);
    return;
  }
  else
    Serial.println("Lora Detected -> LORA OK");

  lora.setDeviceClass(CLASS_C);
  lora.setDataRate(SF10BW125);
  lora.setChannel(MULTI);
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);
#endif

#ifdef ENABLE_ETH
  Ethernet.init(csEthOne);
  Serial.println(F("Starting ethernet..."));
  Ethernet.begin(macEthOne, ipEthOne);
  Serial.println("Trying to get an IP address using DHCP");

  if (Ethernet.begin(macEthOne) == 0)
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    }
    else if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println("Ethernet cable is not connected.");
    }
    else
    {
      Ethernet.begin(macEthOne, ipEthOne);
      Serial.println(F("DEBUG: ETH STATIC: USE STATIC IP"));
      Serial.print(F("      STATIC IP : "));
    }
    // no point in carrying on, so do nothing forevermore:
    //    while (true) {
    //      delay(1);
    //    }
  }
  Serial.println(Ethernet.localIP());
  serverOne.begin();
#endif
}

void loop()
{
  if (!MK10Test)
    MK10Test = MK10SerialTest();

  while (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
      displayGPS();

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
  }

  //   int randomData = random(0, 1000);
  //   if (millis() - previousMillis > interval)
  //   {

  //     sprintf(outStr, "*%d*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%d*%d*%d*%d#",
  //             1234556, 217072470, "0.0000", "0.0000", "45.3880", "0.0000", "0.0000", "0.0009", "0.0000", "0.0000", "21.0000", "0.5270", "50.0050", 810, 36150, 6960, 43110);

  //     Serial.print("Sending: ");
  //     Serial.print(outStr);
  // #ifdef ENABLE_LORA
  //     lora.sendUplink(outStr, strlen(outStr), 0, 1);
  // #endif
  //     Serial.print(F(" w/ RSSI "));
  //     Serial.println(lora.getRssi());
  // #ifdef ENABLE_ETH
  //     serverOne.print("ETH ONE Sent from Arduino : ");
  //     serverOne.println(outStr);
  // #endif
  //     previousMillis = millis();
  //   }
  // #ifdef ENABLE_LORA
  //   lora.update();
  // #endif
}