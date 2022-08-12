#include <Arduino.h>

#define RXD2 16
#define TXD2 17
// defines
// special characters used for msg framing
#include <Arduino.h>
#include <AltSoftSerial.h>

AltSoftSerial altSerial;

// defines
// special characters used for msg framing
#define ACK 0x06  // ACKNOWLEDGE
#define CAN 0x18  // ACKNOWLEDGE
#define ENQ 0x05  // ASCII 0x05 == ENQUIRY
#define STX 0x02  // START OF TEXT
#define ETX 0x03  // END OF TEXT
#define EOT 0x04  // END OF TRANSMISSION
#define LF 0x0A   // LINEFEED
#define NUL 0x00  // NULL termination character
#define XOFF 0x13 // XOFF Pause transmission
#define XON 0x11  // XON Resume transmission
#define DLE 0x10  // DATA LINE ESCAPE
#define CR 0x0D   // CARRIAGE RETURN \r
#define LF 0x0A   // LINE FEED \n

#define CHAR_REGREAD 0x52
#define CHAR_LOGREAD 0x4C
//
#define MSG_INTERVAL 5000            // send an enquiry every 5-sec
#define CHAR_TIMEOUT 2000            // expect chars within a second (?)
#define BUFF_SIZE 20                 // buffer size (characters)
#define MAX_RX_CHARS (BUFF_SIZE - 2) // number of bytes we allow (allows for NULL term + 1 byte margin)
#define SERIALNUM_CHAR 10
#define KWH_CHAR 4
#define KWH_OUT_CHAR 16

#define LED_BLIP_TIME 300 // mS time LED is blipped each time an ENQ is sent

// state names
#define ST_STX 0
#define ST_ETX 1
#define ST_ACK 2
#define ST_CAN 3
#define ST_CON 4
#define ST_LOGIN 5
#define ST_LOGIN_R 6
#define ST_IDLE 7
#define ST_READ 8
#define ST_RX_READ 9
#define ST_LOGOUT 10
#define ST_MSG_TIMEOUT 11

// state reg
#define ST_REG_CON 0
#define ST_REG_SERIAL 1
#define ST_REG_KWH_A 2
#define ST_REG_KWH_B 3
#define ST_REG_KWH_T 4
#define ST_REG_END 5

// constants
// pins to be used may vary with your Arduino for software-serial...
const byte pinLED = LED_BUILTIN; // visual indication of life

unsigned char logonReg[15] = {0x4C, 0x45, 0x44, 0x4D, 0x49, 0x2C, 0x49, 0x4D, 0x44, 0x45, 0x49, 0x4D, 0x44, 0x45, 0x00};
unsigned char logoffReg[1] = {0x58};

byte regSerNum[2] = {0xF0, 0x02};
byte regKWHA[2] = {0x1E, 0x01};
byte regKWHB[2] = {0x1E, 0x02};
byte regKWHTOT[2] = {0x1E, 0x00};

// globals
char
    rxTextFrame[BUFF_SIZE];
char
    dataRX[BUFF_SIZE];

bool
    bLED;
unsigned long
    timeLED;

char outSerialNumber[SERIALNUM_CHAR];
char outKWHA[KWH_OUT_CHAR];
char outKWHB[KWH_OUT_CHAR];
char outKWHTOT[KWH_OUT_CHAR];

void ReceiveStateMachine(void);
bool GetSerialChar(char *pDest, unsigned long *pTimer);
bool CheckTimeout(unsigned long *timeNow, unsigned long *timeStart);

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
  altSerial.write(ch);
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
     Add the STX and start the CRC calc.
  */
  cmdlink_putch(STX);
  crc = CalculateCharacterCRC16(0, STX);
  /*
     Send the data, computing CRC as we go.
  */
  for (i = 0; i < len; i++)
  {
    send_byte(*cmd);
    crc = CalculateCharacterCRC16(crc, *cmd++);
  }
  /*
     Add the CRC
  */
  send_byte((unsigned char)(crc >> 8));
  send_byte((unsigned char)crc);
  /*
     Add the ETX
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

void cmd_readRegister(const byte *reg)
{
  unsigned char readReg[3] = {0x52, reg[0], reg[1]};
  send_cmd(readReg, sizeof(readReg));
}

void parsingCMD(unsigned char *cmd_data, unsigned short max_len)
{

  static char DLE_last;
  unsigned short i;
  short c;
  static unsigned short crc;
  unsigned char currData;
  int posData = 0;

  for (i = 0; i < max_len; i++)
  {
    c = cmd_data[i];
    switch (c)
    {
    case STX:
      currData = cmd_data[i];
      posData = 0;
      crc = CalculateCharacterCRC16(0, c);
      break;

    case ETX:
      if ((crc == 0) && (posData > 2))
      {
        posData -= 2; /* remove crc characters */
        break;
        // return (TRUE);
      }
      else if (posData == 0)
        // return (TRUE);
        break;

    case DLE:
      DLE_last = true;
      break;

    default:
      if (DLE_last)
        c &= 0xBF;
      DLE_last = false;
      if (posData >= max_len)
        break;
      crc = CalculateCharacterCRC16(crc, c);
      currData = c;
      posData++;
      Serial.print("DATA [");
      Serial.print(posData);
      Serial.print("] = ");
      Serial.println(c, HEX);
    }
  }
}

void textFromHexString(char *hex, char *result)
{
  char temp[3];
  int index = 0;
  temp[2] = '\0';
  while (hex[index])
  {
    strncpy(temp, &hex[index], 2);
    *result = (char)strtol(temp, NULL, 16); // If representations are hex
    result++;
    index += 2;
  }
  *result = '\0';
}

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

void ReceiveStateMachine(void)
{
  char
      ch;
  static byte
      rxPtr = 0;
  static byte
      regCnt = 0;
  static byte
      rxCnt = 0;
  static byte
      stateReg = ST_REG_CON,
      stateRegLast = stateReg;
  static unsigned long
      timeMsg = 0,
      timeRX;
  unsigned long
      timeNow;
  static byte
      stateNext,
      stateRX = ST_CON;

  timeNow = millis();
  switch (stateRX)
  {
  case ST_CON:
    if ((timeNow - timeMsg) >= MSG_INTERVAL)
    {
      bLED = true; // turn on the LED for visual indication
      timeLED = timeNow;
      digitalWrite(pinLED, HIGH);
      cmd_trigMeter();
      timeRX = timeNow;   // character timeout
      stateRX = ST_LOGIN; // wait for the start of text
    }                     // if
    break;

  case ST_LOGIN:
    if (GetSerialChar(&ch, &timeRX) == true)
    {
      regCnt++;
      if (ch == ACK /*and regCnt == 2*/)
      {
        Serial.println(F("DEBUG: ST_LOGIN: CONNECT ACK."));
        cmd_logonMeter();
        stateRX = ST_LOGIN_R;
      } // if
    }
    else
    {
      if (CheckTimeout(&timeNow, &timeRX))
      {
        Serial.println(F("DEBUG: ST_LOGIN: TIMEOUT..."));
        stateRX = ST_MSG_TIMEOUT;
      }
    } // else
    break;

  case ST_LOGIN_R:
    if (GetSerialChar(&ch, &timeRX) == true)
    {
      regCnt++;
      if (ch == ACK /*and regCnt == 2*/)
      {
        Serial.println(F("DEBUG: ST_LOGIN_R: LOGIN ACK."));
        stateRX = ST_READ;
      } // if
    }   // if
    else
    {
      if (CheckTimeout(&timeNow, &timeRX))
      {
        Serial.println(F("DEBUG: ST_LOGIN_R: TIMEOUT..."));
        stateRX = ST_MSG_TIMEOUT;
      }
    }
    break;
  case ST_READ:
    Serial.println(F("DEBUG: ST_READ: LOGIN ACK."));
    switch (stateReg)
    {
    case ST_REG_SERIAL:
      Serial.println(F("      READ  - SERIAL NUMBER."));
      cmd_readRegister(regSerNum);
      stateRegLast = stateReg;
      stateRX = ST_RX_READ;
      break;
      //        case ST_REG_KWH_A:
      //          Serial.println(F("      READ  - KWH A."));
      //          cmd_readRegister(regKWHA);
      //          stateRegLast = stateReg;
      //          //          stateRX = ST_RX_READ;
      //          break;
      //        case ST_REG_KWH_B:
      //          Serial.println(F("      READ  - KWH B."));
      //          cmd_readRegister(regKWHB);
      //          stateRegLast = stateReg;
      //          //          stateRX = ST_RX_READ;
      //          break;
      //        case ST_REG_KWH_T:
      //          Serial.println(F("      READ  - KWH TOTAL."));
      //          cmd_readRegister(regKWHTOT);
      //          stateRegLast = stateReg;
      //          //          stateRX = ST_RX_READ;
      //          break;
    }
    stateRX = ST_RX_READ;
    break;

  case ST_RX_READ:
    if (GetSerialChar(&ch, &timeRX) == true)
    {
      // receive another character into the buffer
      rxTextFrame[rxPtr] = ch;
      // protect the buffer from overflow
      if (rxPtr < MAX_RX_CHARS)
        rxPtr++;
      Serial.println
    }
    // if
    else
    {
      // check for a timeout
      if (CheckTimeout(&timeNow, &timeRX))
      {
        Serial.println(F("DEBUG: ST_RX_READ: TIMEOUT..."));
        stateRX = ST_MSG_TIMEOUT;
      }
    } // else
    break;

  case ST_LOGOUT:
    // receive characters into rxBuff until we see a LF
    regCnt = 0;
    if (GetSerialChar(&ch, &timeRX) == true)
    {
      // have we received the EOT token?
      if (ch == ACK)
      {
        Serial.println(F("DEBUG: ST_LOGOUT: LOGOUT ACK"));
        // yes; reset ENQ timer and return to that state
        timeMsg = timeNow;
        stateRX = ST_CON;
      } // if
    }   // if
    else
    {
      // check for a timeout
      if (CheckTimeout(&timeNow, &timeRX))
      {
        Serial.println(F("DEBUG: ST_LOGOUT: TIMEOUT..."));
        stateRX = ST_MSG_TIMEOUT;
      }
    } // else
    break;

  case ST_MSG_TIMEOUT:
    Serial.println(F("DEBUG: ST_MSG_TIMEOUT:")); // send debug msg to serial monitor
    Serial.println(F("      Timeout waiting for character"));

    while (altSerial.available())
      ch = altSerial.read();

    // then reset back to ENQ state
    timeMsg = timeNow;
    stateRX = ST_CON;
    break;
  } // switch
} // ReceiveStateMachine

bool GetSerialChar(char *pDest, unsigned long *pTimer)
{
  if (altSerial.available())
  {
    // get the character
    *pDest = altSerial.read();

    // and reset the character timeout
    *pTimer = millis();

    return true;

  } // if
  else
    return false;

} // GetSerialChar

// CheckTimeout
bool CheckTimeout(unsigned long *timeNow, unsigned long *timeStart)
{
  if (*timeNow - *timeStart >= CHAR_TIMEOUT)
    return true;
  else
    return false;
}

void ServiceLED(void)
{
  if (!bLED)
    return;

  if ((millis() - timeLED) >= LED_BLIP_TIME)
  {
    digitalWrite(pinLED, LOW);
    bLED = false;

  } // if

} // ServiceLED

void hexStringtoASCII(char *hex, char *result)
{
  char text[25] = {0}; // another 25
  int tc = 0;

  for (int k = 0; k < strlen(hex); k++)
  {
    if (k % 2 != 0)
    {
      char temp[3];
      sprintf(temp, "%c%c", hex[k - 1], hex[k]);
      int number = (int)strtol(temp, NULL, 16);
      text[tc] = char(number);
      tc++;
    }
  }
  strcpy(result, text);
}

// uint32_t value = strtoul(strCoreData.c_str(), NULL, 16);
// dtostrf(ConvertB32ToFloat(value), 1, 9, s);

void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(pinLED, OUTPUT);

} // setup

void loop()
{
  ReceiveStateMachine(); // messaging with meter
  ServiceLED();          // for LED timing

} // loop