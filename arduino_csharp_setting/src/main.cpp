#include <Arduino.h>
#include <IPAddress.h>

#define BUFFER_SIZE 20
#define VERSION "1.0"
#define FLAGHEAD 0x3C
#define FLAGTAIL 0x3E
#define REGA 0x41
#define REGE 0x45
#define REGI 0x49
#define REGK 0x4B
#define REGL 0x4C
#define REGM 0x4D
#define REGR 0x52
#define MSG_BUF_SIZE 255
const int INTERCHAR_TIMEOUT = 50;

// Buffer for incoming data
char serial_buffer[BUFFER_SIZE];
int buffer_position;
IPAddress ipStatic;

int ipStringToInt(const char *pDottedQuad, unsigned int *pIpAddr)
{
  unsigned int byte3;
  unsigned int byte2;
  unsigned int byte1;
  unsigned int byte0;
  char dummyString[2];

  /* The dummy string with specifier %1s searches for a non-whitespace char
   * after the last number. If it is found, the result of sscanf will be 5
   * instead of 4, indicating an erroneous format of the ip-address.
   */
  if (sscanf(pDottedQuad, "%u.%u.%u.%u%1s", &byte3, &byte2, &byte1, &byte0,
             dummyString) == 4)
  {
    if ((byte3 < 256) && (byte2 < 256) && (byte1 < 256) && (byte0 < 256))
    {
      *pIpAddr = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + byte0;

      return 1;
    }
  }

  return 0;
}

void printIpFromInt(unsigned int ip)
{
  unsigned char bytes[4];
  char buff[20];
  bytes[0] = ip & 0xFF;
  bytes[1] = (ip >> 8) & 0xFF;
  bytes[2] = (ip >> 16) & 0xFF;
  bytes[3] = (ip >> 24) & 0xFF;
  sprintf("%d.%d.%d.%d\n", buff, bytes[3], bytes[2], bytes[1], bytes[0]);
  Serial.println(buff);
}

void sendMessagePacket(char message[])
{
  int index = 0;
  byte checksum = 0;

  Serial.print((char)FLAGHEAD);
  while (message[index] != NULL)
  {
    Serial.print(message[index]);
  }
  Serial.print((char)FLAGTAIL);
}

boolean readChar(unsigned int timeout, byte *inChar)
{
  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;

  while (currentMillis - previousMillis < timeout)
  {
    if (Serial.available())
    {
      (*inChar) = (byte)Serial.read();
      return true;
    }
    currentMillis = millis();
  } // while
  return false;
}

boolean readMessage(char *message, int maxMessageSize, unsigned int timeout)
{
  byte inChar;
  int index = 0;
  boolean completeMsg = false;

  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;

  while (currentMillis - previousMillis < timeout)
  {
    if (Serial.available())
    {
      inChar = (byte)Serial.read();

      if (inChar == FLAGHEAD) // start of message?
      {
        while (readChar(INTERCHAR_TIMEOUT, &inChar))
        {
          if (inChar != FLAGTAIL)
          {
            message[index] = inChar;
            index++;
            if (index == maxMessageSize) // buffer full
              break;
          }
          else // got FLAGTAIL, next character is the checksum
          {
            message[index] = NULL;
            completeMsg = true;
            break;
          }
        } // while read until FLAGTAIL or timeout
      }   // inChar == FLAGHEAD
    }     // Serial.available()
    if (completeMsg)
      break;
    currentMillis = millis();
  } // while
  return completeMsg;
}

void parseEth(char *str)
{
  const char s[2] = "|";
  char *token;

  /* get the first token */
  token = strtok(str, s);

  /* walk through other tokens */
  while (token != NULL)
  {
    Serial.println(token);

    token = strtok(NULL, s);
  }
}

void setEth()
{
}

void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600);
  IPAddress aa(3232235777);
  Serial.println(aa);
}

void loop()
{
  char msg[MSG_BUF_SIZE];

  if (readMessage(msg, MSG_BUF_SIZE, 5000))
  {
    switch (msg[0])
    {
    case REGA:
      Serial.println("");
      break;
    case REGE:
      // parseEth(msg);
      Serial.println("E192.168.1.1|255.255.255.0|192.168.1.1|8.8.8.8");
      break;
    case REGI:
      Serial.println("I696969696");
      break;
    case REGK:
      Serial.println("");
      break;
    case REGL:
      Serial.println("");
      break;
    case REGM:
      Serial.println("");
      break;
    case REGR:
      Serial.println("");
      break;
    default:
      break;
    }
  }
}