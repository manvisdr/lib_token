#include <Arduino.h>
// #include "Countimer.h"

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

unsigned char logonReg[15] = {0x4C, 0x45, 0x44, 0x4D, 0x49, 0x2C, 0x49, 0x4D, 0x44, 0x45, 0x49, 0x4D, 0x44, 0x45, 0x00};
unsigned char logoffReg[1] = {0x58};

const char *table_mainReg[1] =
    {
        "F010" /*Serial Number*/,
        // "1E01" /*CH1:KWH Total Rate A*/,
        // "1E02" /*CH1:KWH Total Rate B*/,
        // "1E03" /*CH1:KWH Total Rate C*/,
        // "1E00" /*CH1:KWH Total Unified*/,
};

const char *table_mainRegStr[5] =
    {
        "Serial Number" /*Serial Number*/,
        "CH1:KWH Total Rate A" /*CH1:KWH Total Rate A*/,
        "CH1:KWH Total Rate B" /*CH1:KWH Total Rate B*/,
        "CH1:KWH Total Rate C" /*CH1:KWH Total Rate C*/,
        "CH1:KWH Total Unified" /*CH1:KWH Total*/,
};

byte nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0; // Not a valid hexadecimal character
}

void hexCharacterStringToBytes(byte *byteArray, const char *hexString)
{
  bool oddLength = strlen(hexString) & 1;
  byte currentByte = 0;
  byte byteIndex = 0;
  for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
  {
    bool oddCharIndex = charIndex & 1;
    if (oddLength)
    {
      if (oddCharIndex)
      {
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
    else
    {
      if (!oddCharIndex)
      {
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
  }
}

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

// SEND TO KWH
void cmdlink_putch(unsigned char ch)
{
  Serial2.write(ch);
}

void send_byte(unsigned char d)
{
  switch (d)
  {
  case STX:
  case ETX:
  case DLE:
  case XON:
  case XOFF:
    cmdlink_putch(DLE);
    cmdlink_putch(d | 0x40);
    break;
  default:
    cmdlink_putch(d);
  }
}

void send_cmd(unsigned char *cmd, unsigned short len)
{
  unsigned short i;
  unsigned short crc;
  /*
   * Add the STX and start the CRC calc.
   */
  cmdlink_putch(STX);
  crc = CalculateCharacterCRC16(0, STX);
  /*
   * Send the data, computing CRC as we go.
   */
  for (i = 0; i < len; i++)
  {
    send_byte(*cmd);
    crc = CalculateCharacterCRC16(crc, *cmd++);
  }
  /*
   * Add the CRC
   */
  send_byte((unsigned char)(crc >> 8));
  send_byte((unsigned char)crc);
  /*
   * Add the ETX
   */
  cmdlink_putch(ETX);
}

bool cmd_trigMeter()
{
  cmdlink_putch(STX);
  cmdlink_putch(ETX);
  return true;
}

bool cmd_logonMeter()
{
  send_cmd(logonReg, sizeof(logonReg));
  return true;
}

bool cmd_logoffMeter()
{
  send_cmd(logoffReg, sizeof(logoffReg));
  return true;
}

// READ REGISTER COMMAND 'R'
// unsgined char register ==>> F0 02 -> 0xF0 0x02
void cmd_readRegister(const byte *reg)
{
  unsigned char readReg[3] = {0x52, reg[0], reg[1]};
  send_cmd(readReg, sizeof(readReg));
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
char get_cmd(unsigned char *cmd_data, unsigned short *len,
             unsigned short max_len)
{
  short c;
  static unsigned char *cur_pos = (void *)0;
  static unsigned short crc;
  static char DLE_last;
  /*
   * check is cur_pos has not been initialised yet.
   */
  if (!cur_pos)
  {
    cur_pos = cmd_data;
    *len = 0;
  }
  /*
   * Get characters from the serial port while they are avialable
   */
  while ((c = get_char()) != -1)
  {
    switch (c)
    {
    case STX:
      cur_pos = cmd_data;
      *len = 0;
      crc = CalculateCharacterCRC16(0, c);
      break;
    case ETX:
      if ((crc == 0) && (*len > 2))
      {
        *len -= 2; /* remove crc characters */
        return (TRUE);
      }
      else if (*len == 0)
        return (TRUE);
      break;
    case DLE:
      DLE_last = TRUE;
      break;
    default:
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
  return (FALSE);
}

// Countimer tUp;
unsigned char command[3] = {0x52, 0xE0, 0x90};
unsigned char command2[3] = {0x52, 0xF0, 0x10};
const byte sizeRegTable = 2;
byte regTable[sizeRegTable] = {0};
char serIn;
bool logonMeter = false;
bool logoffMeter = true;

void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600);
  // tUp.setCounter(0, 0, 10, tUp.COUNT_UP, tUpComplete);
}
void loop()
{
  // tUp.run();
  // tUp.start();

  // if(now - timeLastInput > TIME_OUT) {
  //         Serial.println("Time out");
  //         index = 0;
  //         timeLastInput = now;
  //     }

  if (Serial2.available() && logonMeter)
  {
    Serial.println("LOGON READ METER");
    for (int i = 0; i < sizeof(table_mainReg); i++)
    {
      hexCharacterStringToBytes(regTable, table_mainReg[i]);
      if (Serial2.available())
      {
        cmd_readRegister(regTable);
        while (Serial2.available() > 0)
        {
          serIn = Serial2.read();   // read Serial
          Serial.print(serIn, HEX); // prints the character just read
        }
      }
      delay(1000);
    }
    logoffMeter = cmd_logoffMeter();
  }
  else if (Serial2.available() && logoffMeter)
  {
    Serial.println("LOGOFF TRY TO LOGON");
    logonMeter = cmd_logonMeter();
  }
  else
  {
    Serial.println("SERIAL NOT AVAILABLE");
    logonMeter = false;
    logoffMeter = true;
    cmd_trigMeter();
  }
}
