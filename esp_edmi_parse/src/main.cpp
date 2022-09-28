#include <Arduino.h>
#include <EDMICmdLine.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
// #include <NTPClient.h>
// #include <PubSubClient.h>
// #include <WiFi.h>
// #include <HTTPClient.h>
// #include <ESPmDNS.h>
// #include <WiFiUdp.h>
// #include <ArduinoOTA.h>

const char *ssid = "MGI-MNC";
const char *password = "#neurixmnc#";
const char *mqtt_server = "203.194.112.238";

// WiFiClient espWifiClient;
// PubSubClient mqttClientWifi(espWifiClient);
// WiFiUDP ntpUDP;
// const long utcOffsetInSeconds = 25200; // GMT INDONESIA FOR NTP
// NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);
// #define TYPE_MK10E

#define RXD2 16 // PIN RX2
#define TXD2 17 // PIN TX2

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

const unsigned int interval_cek_konek = 2000;
const unsigned int interval_record = 15000;
long previousMillis1 = 0;

bool kwhReadReady = false;
String kwhSerialNumber;
const String customerID = "69696969";

IPAddress IPmqtt(203, 194, 112, 238);
String userKey = "das";
String apiKey = "mgi2022";
const String channelId = "monKWH/AMR";
float kwh1;
String allDatas = "";

EdmiCMDReader edmiread(Serial2, RXD2, TXD2);

// bool mqttConnectWifi()
// {
//   static const char alphanum[] = "0123456789"
//                                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//                                  "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
//   char clientId[9];

//   uint8_t retry = 3;
//   while (!mqttClientWifi.connected())
//   {
//     mqttClientWifi.setServer(IPmqtt, 1883);
//     // Serial.println(String("Attempting MQTT broker:") + serverName);

//     for (uint8_t i = 0; i < 8; i++)
//     {
//       clientId[i] = alphanum[random(62)];
//     }
//     clientId[8] = '\0';
//     // Setting mqtt gaes
//     if (mqttClientWifi.connect(clientId, userKey.c_str(), apiKey.c_str()))
//     {
//       // Serial.println("Established:" + String(clientId));
//       return true;
//     }
//     else
//     {
//       if (!--retry)
//         break;
//       // delay(3000);
//     }
//   }
//   return false;
// }

// void mqttPublish(String msg)
// {
//   String path = channelId + customerID;
//   mqttClientWifi.publish(path.c_str(), msg.c_str());
// }

void blink(int times)
{
  for (int x = 0; x <= times; x++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
  }
}
void do_background_tasks()
{

  // timeClient.update();
  // if (!mqttClientWifi.loop()) /* PubSubClient::loop() returns false if not connected */
  // {
  //   mqttConnectWifi();
  // }
  edmiread.keepAlive();
  // Serial.println(timeClient.getFormattedTime());
}

void delay_handle_background()
{
  // Serial.printf("%9s\n", edmiread.printStatus().c_str());
  // (timeClient.getSeconds() == 0)
  if (millis() - previousMillis > interval_cek_konek /* and status == EdmiCMDReader::Status::Connect */)
  {
    // Serial.println(timeClient.getFormattedTime());
    // mqttPublish("*" + customerID + "*" + 385868897 + "*" + 45600 + "*" + 55000 + "*" + 666000 + "#");
    kwhReadReady = true;
    edmiread.acknowledge();
    // Serial.printf("TIME TO READ\n");

    previousMillis = millis();
  }
}

void setup()
{
  Serial.begin(115200);
  edmiread.begin(9600);
  // Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(LED_BUILTIN, OUTPUT);

  edmiread.read_time();
  Serial.printf("%d-%d-%d %d:%d:%d\n",
                edmiread.timeEdmi.day,
                edmiread.timeEdmi.month,
                edmiread.timeEdmi.year,
                edmiread.timeEdmi.hour,
                edmiread.timeEdmi.minute,
                edmiread.timeEdmi.seconds);
  // setTime()
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);

  // kwhSerialNumber = edmiread.serialNumber();
  // Serial.printf("Serial Number - %s\n", kwhSerialNumber);
}

void loop()
{

  // do_background_tasks();
  // edmiread.read_looping();
  // EdmiCMDReader::Status status = edmiread.status();
  // Serial.printf("%s\n", kwhReadReady ? "true" : "false");
  // if (status == EdmiCMDReader::Status::Ready and kwhReadReady)
  // {
  //   blink(3);
  //   Serial.println("STEP_START");
  //   edmiread.step_start();
  // }
  // else if (status == EdmiCMDReader::Status::Finish)
  // {

  //   allDatas = '*' +
  //              kwhSerialNumber +
  //              String(edmiread.voltR(), 4) + '*' +
  //              String(edmiread.voltS(), 4) + '*' +
  //              String(edmiread.voltT(), 4) + '*' +
  //              String(edmiread.currentR(), 4) + '*' +
  //              String(edmiread.currentR(), 4) + '*' +
  //              String(edmiread.currentT(), 4) + '*' +
  //              String(edmiread.wattR(), 4) + '*' +
  //              String(edmiread.wattS(), 4) + '*' +
  //              String(edmiread.wattT(), 4) + '*' +
  //              String(edmiread.pf(), 4) + '*' +
  //              String(edmiread.frequency(), 4) + '*' +
  //              String(edmiread.kVarh(), 0) + '*' +
  //              String(edmiread.kwhLWBP(), 0) + '*' +
  //              String(edmiread.kwhWBP(), 0) + '*' +
  //              String(edmiread.kwhTotal(), 0) +
  //              '#';
  //   mqttPublish(allDatas);

  //   // mqttPublish(allDatas);
  //   Serial.println(allDatas);
  //   Serial.println("FINISH");
  //   blink(5);

  //   kwhReadReady = false;
  // }
  // else if (status != EdmiCMDReader::Status::Busy)
  // {
  //   Serial.println("BUSY");
  //   blink(1);
  // }

  // // digitalWrite(LED_BUILTIN, LOW);
  // delay_handle_background();

} // loop