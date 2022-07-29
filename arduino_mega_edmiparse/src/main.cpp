#include <Arduino.h>
#include "SafeString.h"

#define ENQ 0x05  // ASCII 0x05 == ENQUIRY
#define STX 0x02  // START OF T0xE0T
#define ETX 0x03  // END OF T0xE0T
#define EOT 0x04  // END OF TRANSMISSION
#define LF 0x0A   // LINEFEED
#define NUL 0x00  // NULL termination character
#define XOFF 0x13 // XOFF Pause transmission
#define XON 0x11  // XON Resume transmission
#define DLE 0x10  // DATA LINE ESCAPE
#define CR 0x0D   // CARRIAGE RETURN \r

#define LF 0x0A // LINE FEED \n

#define TRUE 1
#define FALSE 0

int globLenData = 0;
short gencrc_16(short i)
{
  short j;
  short k;
  short crc;
  k = i << 8;
  crc = 0;
  for (j = 0; j < 8; j++)
  {
    if ((crc ^ k) & 0x8000)
      crc = (crc << 1) ^ 0x1021;
    else
      crc <<= 1;
    k <<= 1;
  }
  return (crc);
}

unsigned short
CalculateCharacterCRC16(unsigned short crc, unsigned char c)
{
  return ((crc << 8) ^ gencrc_16((crc >> 8) ^ c));
}

// RECEIVE FROM KWH
short get_char(void)
{
  return (-1);
}
/*
 * Call get_cmd with a data buffer (cmd_data) and the maximum length of
 * the buffer (max_len). get_cmd will return FALSE until a complete
 * command is received. When this happens the length of the data is
 * returned in len. Packets with bad CRC's are discarded.
 */
char get_cmd(unsigned char *cmd_data, unsigned short max_len)
{
  unsigned short i;
  short c;
  static unsigned char *cur_pos = 0;
  static unsigned short crc;
  unsigned char *len;
  static char DLE_last;
  // if (!cur_pos)
  // {
  //   cur_pos = cmd_data;
  //   *len = 0;
  // }

  for (i = 0; i < max_len; i++)
  {
    c = cmd_data[i];
    switch (c)
    {

    case STX:
      // Serial.print(c, HEX);
      // Serial.print(" ");
      // Serial.print("STX");
      // Serial.print(" ");
      cur_pos = cmd_data;
      *len = 0;
      crc = CalculateCharacterCRC16(0, c);
      break;
    case ETX:
      // Serial.print(c, HEX);
      // Serial.print(" ");
      // Serial.println("ETX");
      if ((crc == 0) && (*len > 2))
      {
        *len -= 2; /* remove crc characters */
        return (TRUE);
      }
      else if (*len == 0)
        return (TRUE);
      break;
    case DLE:
      // Serial.print(c, HEX);
      // Serial.print(" ");
      // Serial.print("DLE");
      // Serial.print(" ");
      DLE_last = TRUE;
      break;
    default:
      Serial.print(c, HEX);
      Serial.print(" ");
      if (DLE_last)
        c &= 0xBF;
      DLE_last = FALSE;
      if (*len >= max_len)
        break;
      crc = CalculateCharacterCRC16(crc, c);
      *(cur_pos)++ = c;
      (*len)++;
    }
  }
  Serial.println(" ");
  /*
   * check is cur_pos has not been initialised yet.
   */
}

int *parsingCMD(unsigned char *cmd_data, unsigned short max_len)
{

  static char DLE_last;
  unsigned short i;
  short c;
  static unsigned short crc;
  unsigned char *currData;
  int posData;
  int outData[max_len - 4];
  int cntData;
  globLenData = max_len - 4;

  for (i = 0; i < max_len; i++)
  {
    c = cmd_data[i];
    switch (c)
    {

    case STX:
      currData = cmd_data[i];
      // Serial.println(int(currData));
      posData = 0;
      Serial.print("STX=");
      Serial.println(posData);
      crc = CalculateCharacterCRC16(0, c);
      break;

    case ETX:
      Serial.print("ETX=");
      Serial.println(posData);
      if ((crc == 0) && (posData > 2))
      {
        posData -= 2; /* remove crc characters */
        // return (TRUE);
      }
      else if (posData == 0)
        // return (TRUE);
        break;

    case DLE:
      DLE_last = TRUE;
      break;

    default:
      if (DLE_last)
        c &= 0xBF;
      DLE_last = FALSE;
      if (posData >= max_len)
        break;
      crc = CalculateCharacterCRC16(crc, c);
      currData = c;
      outData[cntData] = c;
      posData++;
      cntData++;
      Serial.print("DATA= ");
      Serial.print(c, HEX);
      Serial.print(" ");
      Serial.println(posData);
      // Serial.print(int(currData));
      // Serial.println(" ");
    }
  }
  return outData;
}

byte aNibble(char in)
{
  if (in >= '0' && in <= '9')
  {
    return in - '0';
  }
  else if (in >= 'a' && in <= 'f')
  {
    return in - 'a' + 10;
  }
  else if (in >= 'A' && in <= 'F')
  {
    return in - 'A' + 10;
  }
  return 0;
}

char *unHex(const char *input, char *target, size_t len)
{
  if (target != nullptr && len)
  {
    size_t inLen = strlen(input);
    if (inLen & 1)
    {
      Serial.println(F("unhex: malformed input"));
    }
    size_t chars = inLen / 2;
    if (chars >= len)
    {
      Serial.println(F("unhex: target buffer too small"));
      chars = len - 1;
    }
    for (size_t i = 0; i < chars; i++)
    {
      target[i] = aNibble(*input++);
      target[i] <<= 4;
      target[i] |= aNibble(*input++);
    }
    target[chars] = 0;
  }
  else
  {
    Serial.println(F("unhex: no target buffer"));
  }
  return target;
}

double ConvertNumberToFloat(unsigned long number, int isDoublePrecision)
{
  int mantissaShift = isDoublePrecision ? 52 : 23;
  unsigned long exponentMask = isDoublePrecision ? 0x7FF0000000000000 : 0x7f800000;
  int bias = isDoublePrecision ? 1023 : 127;
  int signShift = isDoublePrecision ? 63 : 31;

  int sign = (number >> signShift) & 0x01;
  int exponent = ((number & exponentMask) >> mantissaShift) - bias;

  int power = -1;
  double total = 0.0;
  for (int i = 0; i < mantissaShift; i++)
  {
    int calc = (number >> (mantissaShift - i - 1)) & 0x01;
    total += calc * pow(2.0, power);
    power--;
  }
  double value = (sign ? -1 : 1) * pow(2.0, exponent) * (total + 1.0);

  return value;
}

// uint32_t value = strtoul(strCoreData.c_str(), NULL, 16);
// dtostrf(ConvertB32ToFloat(value), 1, 9, s);
float ConvertB32ToFloat(uint32_t b32)
{
  float result;
  int32_t shift;
  uint16_t bias;

  if (b32 == 0)
    return 0.0;
  // pull significand
  result = (b32 & 0x7fffff); // mask significand
  result /= (0x800000);      // convert back to float
  result += 1.0f;            // add one back
  // deal with the exponent
  bias = 0x7f;
  shift = ((b32 >> 23) & 0xff) - bias;
  while (shift > 0)
  {
    result *= 2.0;
    shift--;
  }
  while (shift < 0)
  {
    result /= 2.0;
    shift++;
  }
  // sign
  result *= (b32 >> 31) & 1 ? -1.0 : 1.0;
  return result;
}

bool getDataSerial(char *pDest, unsigned long *pTimer)
{
  if (Serial2.available())
  {
    // get the character
    *pDest = Serial2.read();
    return true;
  } // if
  else
    return false;
}

byte result = 0;
byte c = 0;
byte currData = 0;
int posData = 0;
int aa = 0;
char charData;

createSafeString(stringData, 50);
createSafeString(stringCRC, 4);
void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600);
  SafeString::setOutput(Serial); // enable full debugging error msgs

  // get_cmd(serialnumber, sizeof(serialnumber));
  // out = parsingCMD(serialnumber, sizeof(serialnumber));
  // for (int i = 0; i < 10; i++)
  // {
  //   Serial.println(*out);
  // }
}
void loop()
{

  if (Serial2.available())
  {
    while (Serial2.available() > 0)
    { // receive and collect data

      result = Serial2.read();
      c = result;
      static unsigned short crc;
      static char DLE_last;
      switch (c)
      {
      case STX:
        posData = 0;
        crc = CalculateCharacterCRC16(0, c);
        break;

      case ETX:
        if ((crc == 0) && (posData > 2))
        {
          posData -= 2; /* remove crc characters */
          // return (TRUE);
        }
        else if (posData == 0)
          // return (TRUE);
          break;

      case DLE:
        DLE_last = TRUE;
        break;

      default:
        if (DLE_last)
          c &= 0xBF;
        DLE_last = FALSE;
        if (posData >= posData + 1)
          break;
        crc = CalculateCharacterCRC16(crc, c);
        currData = c;
        stringData += "0123456789ABCDEF"[c / 16];
        stringData += "0123456789ABCDEF"[c % 16];
        posData++;
      }
    }
    c = 0;

    createSafeString(strModeReg, 2);
    stringData.substring(strModeReg, 0, 2);
    createSafeString(strListReg, 4);
    stringData.substring(strListReg, 2, 6);
    createSafeString(strCoreData, stringData.length());

    if (strModeReg == "06")
    {
      Serial.print(F("DATA ASLI "));
      Serial.println(stringData);
      Serial.println(F("CODE 06 = ACK ==> COMMAND SUCCESSFUL"));
    }

    if (strModeReg == "18")
    {
      Serial.print(F("DATA ASLI "));
      Serial.println(stringData);
      Serial.println(F("CODE 18 = CAN ==> UNSUCCESSFUL"));
      Serial.print(F("DATA "));
      stringData.substring(strCoreData, 0, stringData.length() - 4);
      Serial.println(strCoreData);
    }
    if (strModeReg == "52" or strModeReg == "12")
    {
      Serial.print(F("DATA ASLI "));
      Serial.println(stringData);
      Serial.println(F("CODE 52 = FUNTCION R ==> READING"));
      Serial.print(F("REGISTER "));
      stringData.substring(strListReg, 2, 6);
      Serial.println(strListReg);
      Serial.print(F("DATA "));
      stringData.substring(strCoreData, 6, stringData.length() - 4);
      Serial.println(strCoreData);
      if (strListReg == "F002")
      {
        char output[strCoreData.length()];
        Serial.println(unHex(strCoreData.c_str(), output, sizeof(output)));
      }
      else
      {
        char s[16];
        uint32_t value = strtoul(strCoreData.c_str(), NULL, 16);
        dtostrf(ConvertB32ToFloat(value), 1, 9, s);
        Serial.println(s);
      }
      stringData.clear();
    }
  }
}