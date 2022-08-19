#include <EDMICmdLine.h>

#define NUL 0x00  // NULL termination character
#define STX 0x02  // START OF TEXT
#define ETX 0x03  // END OF TEXT
#define EOT 0x04  // END OF TRANSMISSION
#define ENQ 0x05  // ASCII 0x05 == ENQUIRY
#define ACK 0x06  // ACKNOWLEDGE
#define LF 0x0A   // LINE FEED \n
#define CR 0x0D   // CARRIAGE RETURN \r
#define DLE 0x10  // DATA LINE ESCAPE
#define XON 0x11  // XON Resume transmission
#define XOFF 0x13 // XOFF Pause transmission
#define CAN 0x18  // CANCEL

#define R_FUNC 0x52 // READ REGISTER
#define L_FUNC 0x4C // LOGIN REGISTER

#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof(x[0]))

// LOGIN - LEDMI,IMDEIMDE0
const unsigned char register_login[15] = {0x4C, 0x45, 0x44, 0x4D, 0x49, 0x2C, 0x49, 0x4D, 0x44, 0x45, 0x49, 0x4D, 0x44, 0x45, 0x00};
const unsigned char register_serialNum[] = {0xF0, 0x02};
const unsigned char register_bP[] = {0x1E, 0x01};
const unsigned char register_LbP[] = {0x1E, 0x02};
const unsigned char register_Total[] = {0x1E, 0x00};
const unsigned char register_logout[15] = {0x58};

enum class EdmiCMDReader::ErrorCode : uint8_t
{
  NullError,
  CannotWrite,
  UnimplementedOperation,
  RegisterNotFound,
  AccessDenied,
  WrongLength,
  BadTypCode,
  DataNotReady,
  OutOfRange,
  NotLoggedIn,
};

enum class EdmiCMDReader::Step : uint8_t
{
  Ready,
  Started,
  Login,
  NotLogin,
  Read,
  MultiRead,
  Logout,
  Finished
};

static short
gencrc_16(short i)
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
CalculateCharacterCRC16(unsigned short crc, unsigned char c)
{
  return ((crc << 8) ^ ccitt_16[((crc >> 8) ^ c)]);
}

float ConvertB32ToFloat(uint32_t b32)
{
  float
      result;
  int32_t
      shift;
  uint16_t
      bias;

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

uint32_t bytearrayto32byte(char *data)
{
  return ((uint32_t)data[0]) << 24 |
         ((uint32_t)data[1]) << 16 |
         ((uint32_t)data[2]) << 8 |
         ((uint32_t)data[3]);
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
    // Serial.printf("%02X", crc, " ");
  }
  crc = (crc << 8) ^ ccitt_16[((crc >> 8) ^ nTemp)];
  // Serial.printf("crc16_ccitt - ");
  // Serial.printf("%02X", crc, " ");
  // Serial.printf("\n");
  return crc;
}

bool checkCRC(uint8_t *buf, uint16_t len)
{
  if (len <= 3)
    return false;

  uint16_t crc = CRC16buf(buf, len - 2); // Compute CRC of data
  return ((uint16_t)buf[len - 2] << 8 | (uint16_t)buf[len - 1]) == crc;
}

void EdmiCMDReader::begin(unsigned long baud)
{
  serial_.begin(baud, SERIAL_8N1, rx_, tx_);
}

void EdmiCMDReader::TX_raw(unsigned char ch)
{
  serial_.write(ch);
  serial_.flush();
}

void EdmiCMDReader::TX_byte(unsigned char d)
{
  switch (d)
  {
  case STX:
  case ETX:
  case DLE:
  case XON:
  case XOFF:
    EdmiCMDReader::TX_raw(DLE);
    EdmiCMDReader::TX_raw(d | 0x40);
    break;
  default:
    EdmiCMDReader::TX_raw(d);
  }
}

void EdmiCMDReader::TX_cmd(unsigned char *cmd, unsigned short len)
{
  unsigned short i;
  unsigned short crc;
  /*
     Add the STX and start the CRC calc.
  */
  EdmiCMDReader::TX_raw(STX);
  crc = CalculateCharacterCRC16(0, STX);
  /*
     Send the data, computing CRC as we go.
  */
  for (i = 0; i < len; i++)
  {
    EdmiCMDReader::TX_byte(*cmd);
    crc = CalculateCharacterCRC16(crc, *cmd++);
  }
  /*
     Add the CRC
  */
  EdmiCMDReader::TX_byte((unsigned char)(crc >> 8));
  EdmiCMDReader::TX_byte((unsigned char)crc);
  /*
     Add the ETX
  */
  EdmiCMDReader::TX_raw(ETX);
}

void EdmiCMDReader::edmi_R_FUNC(const byte *reg)
{
  unsigned char registers[3] = {R_FUNC, reg[0], reg[1]};
  TX_cmd(registers, sizeof(registers));
}

bool EdmiCMDReader::RX_char(unsigned int timeout, byte *inChar)
{
  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;

  while (currentMillis - previousMillis < timeout)
  {
    if (serial_.available())
    {
      (*inChar) = (byte)serial_.read();
      return true;
    }
    currentMillis = millis();
  } // while
  return false;
}

uint8_t EdmiCMDReader::RX_message(char *message, int maxMessageSize, unsigned int timeout)
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
    if (serial_.available())
    {
      inChar = (byte)serial_.read();

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
              if (checkCRC((uint8_t *)message, index))
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
    }       // serial_.available()
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

void EdmiCMDReader::keepAlive()
{
  if (status_ == Status::Busy)
    return;
  char
      charAck[CHAR_RX_ACK];
  TX_raw(STX);
  TX_raw(ETX);
  RX_message(charAck, CHAR_RX_ACK, RX_TIMEOUT);
  if (charAck[0] == ACK)
    status_ = Status::Connect;
  else
    status_ = Status::Disconnect;
}

void EdmiCMDReader::step_start()
{
  if (status_ == Status::Busy)
    return;

  status_ = Status::Busy;
  step_ = Step::Started;
}

void EdmiCMDReader::step_login()
{
  if (status_ != Status::Busy)
    return;
  char
      charAck[CHAR_RX_ACK];
  TX_cmd((unsigned char *)register_login, sizeof(register_login));
  RX_message(charAck, CHAR_RX_ACK, RX_TIMEOUT);
  if (charAck[0] == ACK)
  {
    if (step_ == Step::Started)
      step_ = Step::Read;
    else
      status_ = Status::LoggedIn;
  }
  //
  // else
  //   status_ = Status::NotLogin;
}

void EdmiCMDReader::step_logout()
{
  char
      charAck[CHAR_RX_ACK];
  TX_cmd((unsigned char *)register_logout, sizeof(register_logout));
  RX_message(charAck, CHAR_RX_ACK, RX_TIMEOUT);
  if (charAck[0] == ACK)
  {
    step_ = Step::Finished;
    status_ = Status::Finish;
  }
}

String EdmiCMDReader::edmi_R_serialnumber(/*char *output, int len*/)
{
  bool
      getSerialSuccess = false;
  char
      charData[CHAR_RX_DATA];
  size_t
      len;
  String output;

  step_login();
  if (status_ == Status::LoggedIn)
  {
    edmi_R_FUNC(register_serialNum);
    len = RX_message(charData, CHAR_RX_DATA, RX_TIMEOUT);
    // Serial.println(len);
    if (charData[0] == CAN)
    {
      regError_ = (ErrorCode)charData[1];
      getSerialSuccess = false;
    }
    else if (charData[0] == R_FUNC and charData[1] == register_serialNum[0] and charData[2] == register_serialNum[1])
    {
      for (size_t i = 0; i < len - 3; i++)
      {
        output += charData[i + 3];
      }
      // Serial.println(output);
      getSerialSuccess = true;
    }
    else
      getSerialSuccess = false;
  }
  return output;
}

void EdmiCMDReader::loop()
{
  if (status_ != Status::Busy or status_ == Status::Disconnect)
    return;
  Serial.println("EdmiCMDReader::loop()");
  switch (step_)
  {
  case Step::Ready:
    break;
  case Step::Started:
    step_login();
    break;
  case Step::Read:
    read_default();
    break;
  case Step::Logout:
    step_logout();
    break;

  default:
    break;
  }
}

bool EdmiCMDReader::read_default()
{
  char
      charData[CHAR_RX_DATA];
  char
      charBuffer[CHAR_DATA_FLOAT];
  size_t
      len;
  if (status_ == Status::Busy)
  {
    edmi_R_FUNC(register_bP);
    len = RX_message(charData, CHAR_RX_DATA, RX_TIMEOUT);
    Serial.println(charData);
    if (charData[0] == CAN)
    {
      regError_ = (ErrorCode)charData[1];
      // return;
    }
    else if (charData[0] == R_FUNC and charData[1] == register_bP[0] and charData[2] == register_bP[1])
    {
      for (size_t i = 0; i < len - 3; i++)
      {
        charBuffer[i] = charData[i + 3];
      }
    }
    edmi_R_FUNC(register_LbP);
    len = RX_message(charData, CHAR_RX_DATA, RX_TIMEOUT);
    Serial.println(charData);
    if (charData[0] == CAN)
    {
      regError_ = (ErrorCode)charData[1];
      // return;
    }
    else if (charData[0] == R_FUNC and charData[1] == register_bP[0] and charData[2] == register_bP[1])
    {
      for (size_t i = 0; i < len - 3; i++)
      {
        charBuffer[i] = charData[i + 3];
      }
      Serial.println(ConvertB32ToFloat(bytearrayto32byte(charBuffer)));
    }
    edmi_R_FUNC(register_bP);
    len = RX_message(charData, CHAR_RX_DATA, RX_TIMEOUT);
    Serial.println(charData);
    if (charData[0] == CAN)
    {
      regError_ = (ErrorCode)charData[1];
      // return;
    }
    else if (charData[0] == R_FUNC and charData[1] == register_Total[0] and charData[2] == register_Total[1])
    {
      for (size_t i = 0; i < len - 3; i++)
      {
        charBuffer[i] = charData[i + 3];
      }
    }
  }
  step_ = Step::Logout;
  return true;
}

String EdmiCMDReader::serialNumber()
{
  if (!read_default()) // Update vales if necessary
    return String(""); // Update did not work, return NAN

  return _currentValues.serialNumber;
}

float EdmiCMDReader::kVarh()
{
  if (!read_default())
    return NAN;

  return _currentValues.kVarh;
}

float EdmiCMDReader::kwhWBP()
{
  if (!read_default())
    return NAN;

  return _currentValues.kwhWBP;
}

float EdmiCMDReader::kwhLWBP()
{
  if (!read_default())
    return NAN;

  return _currentValues.kwhLWBP;
}

float EdmiCMDReader::kwhTotal()
{
  if (!read_default())
    return NAN;

  return _currentValues.kwhTotal;
}
