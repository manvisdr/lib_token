#include <Arduino.h>
#include <SafeString.h>
#include <EDMICmdLine.h>

#define RXD2 16 // PIN RX2
#define TXD2 17 // PIN TX2

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
#define MAX_ACK_CHAR 4
#define KWH_CHAR 4
#define KWH_OUT_CHAR 16

#define LED_BLIP_TIME 300 // mS time LED is blipped each time an ENQ is sent

// state names
#define ST_CON 0
#define ST_LOGIN 1
#define ST_LOGIN_R 2
#define ST_READ 3
#define ST_RX_READ 4
#define ST_LOGOUT 5
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

byte regSerNum[2] = {0xF0, 0x02}; // REGISTER SERIAL NUMBER
byte regKWHA[2] = {0x1E, 0x01};   // REGISTER KWH A
byte regKWHB[2] = {0x1E, 0x02};   // REGISTER KWH B
byte regKWHTOT[2] = {0x1E, 0x00}; // REGISTER KWH TOTAL

uint32_t const READ_DELAY = 1000; /* ms */
unsigned int interval = 5000;
long previousMillis = 0;
// globals
char
    rxBuffer[14];
unsigned char
    dataRX[BUFF_SIZE];
char
    hexNow;

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

uint32_t convertTo32(uint8_t *id)
{
  uint32_t bigvar = (id[0] << 24) + (id[1] << 16) + (id[2] << 8) + (id[3]);
  return bigvar;
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
  Serial2.flush();
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
  Serial2.flush();
}

short get_char(void)
{
  short rx;
  if (Serial2.available())
  {
    // Serial.println("get_char");
    rx = Serial2.read();
    delay(100);
    return rx;
  }
  else
  {
    Serial.println("no get_char");
    return (-1);
  }
}

bool get_cmd(unsigned short max_len)
{
  short c;
  static unsigned char *cur_pos = 0;
  static unsigned short crc;
  static char DLE_last;
  int cntreg = 0;
  /*
   * check is cur_pos has not been initialised yet.
   */

  /*
   * Get characters from the serial port while they are avialable
   */
  while ((c = get_char()) != -1)
  {
    switch (c)
    {
    case STX:
      Serial.println("STX");

      crc = CalculateCharacterCRC16(0, c);
      break;
    case ETX:
      Serial.println("ETX");
      if ((crc == 0) && (cntreg > 2))
      {
        cntreg -= 2; /* remove crc characters */
        return (true);
      }
      else if (cntreg == 0)
        return (true);
      break;
    case DLE:
      Serial.println("DLE");
      DLE_last = true;
      break;
    default:
      if (DLE_last)
        c &= 0xBF;
      DLE_last = false;
      // if (*len >= max_len)
      //   break;
      crc = CalculateCharacterCRC16(crc, c);
      rxBuffer[cntreg] = c;
      Serial.print("POS - ");
      Serial.print(cntreg);
      Serial.print(" DATA - ");
      Serial.println(c, HEX);
      // Serial.print(" rxBUFFER - ");
      // Serial.println(rxBuffer[cntreg], HEX);

      // *(cur_pos)++ = c;
      // (*len)++;
      cntreg++;
    }
  }
  cntreg = 0;
  return (false);
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

typedef union
{
  byte array[4];
  float value;
} ByteToFloat;

float convert(byte *data)
{
  ByteToFloat converter;
  for (byte i = 0; i < 4; i++)
  {
    converter.array[3 - i] = data[i]; // or converter.array[i] = data[i]; depending on endianness
  }
  return converter.value;
}

enum class kwhStatus : uint8_t
{
  Ready,
  Busy,
  Ok,
  Timeout,
  Notlogin,
  ProtocolError,
  ChecksumError,
};

enum class kwhStep : uint8_t
{
  Ready,
  Started,
  SendConnect,
  Connected,
  SendLogin,
  LoginACK,
  SendRegister,
  ReadRegister,
  AfterData,
  SendLogout,
  LogoutACK,
};

enum class kwhRegisterStep : uint8_t
{
  SerialNumber,
  KWHA,
  KWHB,
  KWHTOTAL,
};

size_t errors_ = 0, checksum_errors_ = 0, successes_ = 0;
kwhStatus status_;
kwhStep step_;
kwhRegisterStep regStep_;

void kwh_ready()
{
  if (status_ != kwhStatus::Busy)
    status_ = kwhStatus::Ready;
}

void kwh_start_reading()
{
  /* Don't allow starting a read when one is already in progress */
  if (status_ == kwhStatus::Busy)
    return;

  status_ = kwhStatus::Busy;
  step_ = kwhStep::Started;
  regStep_ = kwhRegisterStep::KWHA;
}

void kwh_change_status(kwhStatus to)
{
  if (to == kwhStatus::ProtocolError)
    ++errors_;
  else if (to == kwhStatus::ChecksumError)
    ++checksum_errors_;
  else if (to == kwhStatus::Ok)
    ++successes_;

  status_ = to;
}

void kwh_SendConnect()
{
  cmd_trigMeter();
  Serial2.flush();
  delay(100);
  Serial.println(F("kwh_SendConnect()"));
  step_ = kwhStep::Connected;
}

void kwh_sendLogin()
{
  Serial.println(F("kwh_sendLogin()"));
  cmd_logonMeter();
  Serial2.flush();
  delay(100);
  Serial.println(F("SEND LOGIN"));
  step_ = kwhStep::LoginACK;
}

void kwh_readACK()
{
  Serial.print(F("kwh_readACK() -> "));
  char charACK[MAX_ACK_CHAR];
  char flushRX;
  if (Serial2.available() > 0)
  {
    flushRX = Serial2.read();
  }
  size_t len = Serial2.readBytesUntil(ETX, charACK, MAX_ACK_CHAR);

  if (charACK[0] == ACK)
  {
    if (step_ == kwhStep::Connected)
    {
      Serial.println(F("ACK RECEIVED - kwhStep::Connected"));
      step_ = kwhStep::SendLogin;
    }
    else if (step_ == kwhStep::LoginACK)
    {
      Serial.println(F("ACK RECEIVED - kwhStep::LoginACK"));
      step_ = kwhStep::SendRegister;
    }
    else if (step_ == kwhStep::AfterData)
    {
      Serial.println(F("ACK RECEIVED - kwhStep::AfterData"));
      step_ = kwhStep::LogoutACK;
    }
  }
}

void kwh_parseData()
{
  Serial.println(F("kwh_parseData()"));
  char charData[MAX_RX_CHARS];
  char buffData[KWH_CHAR];
  createSafeString(outParse, 8);
  char flushRX;

  char s[16];
  uint32_t value;
  // uint32_t nevlaue;
  // long x;
  // float y;
  // uint32_t *const py = (uint32_t *)&y;

  size_t len = Serial2.readBytesUntil(ETX, charData, MAX_RX_CHARS);

  Serial.print(F("LEN - "));
  Serial.println(len);
  Serial.println(charData);

  switch (regStep_)
  {

  case kwhRegisterStep::KWHA:
    Serial.println(F("     DATA KWH_A "));
    for (size_t i = 0; i < KWH_CHAR; i++)
    {
      buffData[i] = charData[i + 4];
      outParse += "0123456789ABCDEF"[buffData[i] / 16];
      outParse += "0123456789ABCDEF"[buffData[i] % 16];
      Serial.print(F("     DATA ke "));
      Serial.print(i);
      Serial.print(F(" > "));
      Serial.print(charData[i + 4], HEX);
      Serial.print(F(" | BUFFDATA ="));
      Serial.println(buffData[i], HEX);
    }
    Serial.print(F("     "));
    Serial.println(buffData);
    // Serial.print(convert((byte)buffData)); using union
    // outKWHA = convert((byte)buffData);

    Serial.println(outParse);
    value = strtoul(outParse.c_str(), NULL, 16);
    // dtostrf(convert((byte)buffData), 1, 0, outKWHA);
    Serial.println(outKWHA);
    outParse.clear();

    break;

  case kwhRegisterStep::KWHB:
    Serial.println(F("     DATA KWH_B "));
    for (size_t i = 0; i < KWH_CHAR; i++)
    {
      buffData[i] = charData[i + 5];
      outParse += "0123456789ABCDEF"[buffData[i] / 16];
      outParse += "0123456789ABCDEF"[buffData[i] % 16];
      Serial.print(F("     DATA ke "));
      Serial.print(i);
      Serial.print(F(" > "));
      Serial.print(charData[i + 5], HEX);
      Serial.print(F(" | BUFFDATA ="));
      Serial.println(buffData[i], HEX);
    }
    Serial.print(F("     "));
    Serial.println(buffData);

    Serial.println(outParse);
    value = strtoul(outParse.c_str(), NULL, 16);
    // dtostrf(convert((byte)buffData), 1, 0, outKWHB);
    Serial.println(outKWHB);
    outParse.clear();

    break;

  case kwhRegisterStep::KWHTOTAL:
    Serial.println(F("     DATA KWH_T "));
    for (size_t i = 0; i < KWH_CHAR; i++)
    {
      buffData[i] = charData[i + 4];
      outParse += "0123456789ABCDEF"[buffData[i] / 16];
      outParse += "0123456789ABCDEF"[buffData[i] % 16];
      Serial.print(F("     DATA ke "));
      Serial.print(i);
      Serial.print(F(" > "));
      Serial.print(charData[i + 4], HEX);
      Serial.print(F(" | BUFFDATA ="));
      Serial.println(buffData[i], HEX);
    }
    Serial.print(F("     "));
    Serial.println(buffData);

    Serial.println(outParse);
    value = strtoul(outParse.c_str(), NULL, 16);
    // dtostrf(convert((byte)buffData), 1, 0, outKWHTOT);
    Serial.println(outKWHTOT);
    outParse.clear();

    break;
  }
  memset(charData, 0x00, MAX_RX_CHARS); // array is reset to 0s.
}

void kwh_readRegister()
{
  switch (regStep_)
  {
  case kwhRegisterStep::KWHA:
    Serial.println(F("READ KWHA"));
    kwh_parseData();
    step_ = kwhStep::SendRegister;
    regStep_ = kwhRegisterStep::KWHB;
    break;
  case kwhRegisterStep::KWHB:
    Serial.println(F("READ KWHB"));
    kwh_parseData();
    step_ = kwhStep::SendRegister;
    regStep_ = kwhRegisterStep::KWHTOTAL;
    break;
  case kwhRegisterStep::KWHTOTAL:
    Serial.println(F("READ KWHTOTAL"));
    kwh_parseData();
    step_ = kwhStep::AfterData;
    break;
  }
}

void kwh_sendRegister()
{
  switch (regStep_)
  {
  case kwhRegisterStep::KWHA:
    Serial.println(F("SEND REGISTER KWHA"));
    cmd_readRegister(regKWHA);
    Serial2.flush();
    delay(100);
    step_ = kwhStep::ReadRegister;
    break;
  case kwhRegisterStep::KWHB:
    Serial.println(F("SEND REGISTER KWHB"));
    cmd_readRegister(regKWHB);
    Serial2.flush();
    delay(100);
    step_ = kwhStep::ReadRegister;
    break;
  case kwhRegisterStep::KWHTOTAL:
    Serial.println(F("SEND REGISTER KWH TOTAL"));
    cmd_readRegister(regKWHTOT);
    Serial2.flush();
    delay(100);
    step_ = kwhStep::ReadRegister;
    break;
  }
}

void kwh_sendLogout()
{
  Serial.println(F("SEND LOGOUT"));
  cmd_logoffMeter();
  kwh_readACK();
}

void kwh_lastStep()
{
  Serial.println(F("KWH LOGOUT -> ACK"));
  return kwh_change_status(kwhStatus::Ok);
}

void kwh_loop()
{
  if (status_ != kwhStatus::Busy)
    return;

  switch (step_)
  {
  case kwhStep::Ready: /* nothing to do, this should never happen */
    break;
  case kwhStep::Started:
    kwh_SendConnect();
    break;
  case kwhStep::Connected:
    kwh_readACK();
    break;
  case kwhStep::SendLogin:
    kwh_sendLogin();
    break;
  case kwhStep::LoginACK:
    kwh_readACK();
    break;
  case kwhStep::SendRegister:
    kwh_sendRegister();
    break;
  case kwhStep::ReadRegister:
    kwh_readRegister();
    break;
  case kwhStep::AfterData:
    kwh_sendLogout();
  case kwhStep::LogoutACK:
    kwh_lastStep();
    break;
  }
}

bool kwh_getSerialNumber()
{
  bool getSerialSuccess = false;
  Serial.print(F("kwh_getSerialNumber() -> "));
  cmd_logonMeter();
  char charACK[MAX_ACK_CHAR];
  char charData[MAX_RX_CHARS];
  char flushRX;
  if (Serial2.available() > 0)
  {
    flushRX = Serial2.read();
  }
  size_t len = Serial2.readBytesUntil(ETX, charACK, MAX_ACK_CHAR);
  Serial.println(len);
  Serial.println(charACK[0]);
  if (charACK[1] == CAN)
  {
    Serial.println(F("CAN CANCEL."));
    getSerialSuccess = false;
  }
  else if (charACK[1] == ACK)
  {
    Serial.println(F("ACK RECEIVED"));
    Serial.println(F("      READ  - SERIAL NUMBER."));
    cmd_readRegister(regSerNum);
    if (Serial2.available() > 0)
    {
      flushRX = Serial2.read();
    }
    size_t lens = Serial2.readBytesUntil(ETX, charData, MAX_RX_CHARS);
    Serial.println(lens);
    Serial.println(charData[1]);
    if (charData[1] == CHAR_REGREAD)
    {
      Serial.println(F("     DATA SERIAL NUMBER"));
      for (size_t i = 0; i < sizeof(outSerialNumber); i++)
      {
        outSerialNumber[i] = charData[i + 5];
        Serial.print(F("     DATA ke "));
        Serial.print(i);
        Serial.print(F(" > "));
        Serial.println(outSerialNumber[i], HEX);
      }
      Serial.print(F("     "));
      Serial.println(outSerialNumber);
      getSerialSuccess = true;
    }
    else
      getSerialSuccess = false;
  }
  return getSerialSuccess;
}
const unsigned int interval = 5000;
const unsigned int interval_cek_konek = 2000;
const unsigned int interval_record = 10000;
long previousMillis = 0;
long previousMillis1 = 0;
void do_background_tasks()
{
  // if (!mqtt.loop()) /* PubSubClient::loop() returns false if not connected */
  // {
  //   mqtt_connect();
  // }
  EdmiCMDReader::Status status = edmiread.status();
  edmiread.step_trigger();
  if (millis() - previousMillis > interval_record and status == EdmiCMDReader::Status::Connect)
  {
    edmiread.read_default();
    previousMillis = millis();
  }
}

void delay_handle_background(long time)
{
  long const increment = 5;
  while ((time -= increment) > 0)
  {
    // do_background_tasks();
    delay(increment);
  }
}

boolean readChar(unsigned int timeout, byte *inChar)
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

boolean readMessage(char *message, int maxMessageSize, unsigned int timeout)
{
  byte inChar;
  int index = 0;
  boolean completeMsg = false;
  byte checksum = 0;
  bool dlechar = false;

  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;

  while (currentMillis - previousMillis < timeout)
  {
    if (Serial2.available())
    {
      inChar = (byte)Serial2.read();

      if (inChar == STX) // start of message?
      {
        while (readChar(INTERCHAR_TIMEOUT, &inChar))
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
            // checksum = checksum + inChar;
          }
          else // got ETX, next character is the checksum
          {
            message[index] = '\0'; // good checksum, null terminate message
            completeMsg = true;

          } // next character is the checksum
        }   // while read until ETX or timeout
      }     // inChar == STX
    }       // Serial2.available()
    if (completeMsg)
      break;
    currentMillis = millis();
  } // while
  return completeMsg;
}

EdmiCMDReader edmiread(Serial2, RXD2, TXD2, "MK10E");
const std::map<EdmiCMDReader::Status, std::string> METER_STATUS_MAP = {
    {EdmiCMDReader::Status::Disconnect, "Disconnect"},
    {EdmiCMDReader::Status::Connect, "Connect"},
    {EdmiCMDReader::Status::Ready, "Ready"},
    {EdmiCMDReader::Status::LoggedIn, "LoggedIn"},
    {EdmiCMDReader::Status::NotLogin, "NotLogin"},
    {EdmiCMDReader::Status::Busy, "Busy"},
    {EdmiCMDReader::Status::Finish, "Finish"},
    {EdmiCMDReader::Status::TimeoutError, "Err-Timeout"},
    {EdmiCMDReader::Status::ProtocolError, "Err-Prot"},
    {EdmiCMDReader::Status::ChecksumError, "Err-Chk"}};

std::string EdmiCMDReaderStatus()
{
  EdmiCMDReader::Status status = edmiread.status();
  auto it = METER_STATUS_MAP.find(status);
  if (it == METER_STATUS_MAP.end())
    return "Unknw";
  return it->second;
}

char gg;
bool getKWH = false;
bool getKWHser = false;
int count = 0;
String kwhSerialNumber;

void setup()
{
  Serial.begin(115200);
  edmiread.begin(9600);
  // Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(pinLED, OUTPUT);

  // getKWHser = kwh_getSerialNumber();
  // Serial.println(getKWHser);
}

void loop()
{

  do_background_tasks();

  // edmiread.step_trigger();
  // Serial.printf("%9s\n", EdmiCMDReaderStatus().c_str());
  // delay(200);
  // if (kwhSerialNumber.length() == 0)
  //   kwhSerialNumber = edmiread.edmi_R_serialnumber();
  // Serial.println(kwhSerialNumber);
  // delay(200);

} // loop