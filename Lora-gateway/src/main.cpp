#include <lorawan.h>
#include <TinyGPS++.h>
TinyGPSPlus gps;
#define AU_915
#define gpsSerial Serial2
// ABP Credentials
const char *devAddr = "00aedb72";
const char *nwkSKey = "1FD053B476BF4F732DC8CF5E565538B9";
const char *appSKey = "2EE8E2940E6799AD78F9F4C6DA8C53D1";

const unsigned long interval = 5000; // 10 s interval to send message
unsigned long previousMillis = 0;    // will store last time message sent
unsigned int counter = 0;            // message counter

char myStr[255];
char outStr[100];
byte recvStatus = 0;

String str_lat = "";
String str_lon = "";
String str_date = "";
String str_time = "";

const sRFM_pins RFM_pins = {
    .CS = 5,
    .RST = 26,
    .DIO0 = 25,
    .DIO1 = 32,
    // .DIO2 = -1,
    // .DIO5 = -1,
};

void displayInfo()
{
  String str_buf_lat = "";
  String str_buf_lon = "";
  String str_buf_date = "";
  String str_buf_time = "";

  Serial.print(F("Location: "));
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
    str_buf_lat = String(gps.location.lat(), 6);
    str_buf_lat = String(gps.location.lng(), 6);
    str_lat = str_buf_lat;
    str_lon = str_buf_lon;
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
    str_buf_date = String(gps.date.month()) + "/" + String(gps.date.day()) + "/" + String(gps.date.year());
    str_date = str_buf_date;
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    str_buf_time = String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
    str_time = str_buf_time;
    Serial.print(str_time);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}

double lat;
double lng;
char latBuffer[20];
char lngBuffer[20];
char payload[100];

void setup()
{
  Serial.begin(115200);
  Serial.println("Start..");
  gpsSerial.begin(9600, SERIAL_8N1, 21, 22);
  // if (!lora.init())
  // {
  //   Serial.println("RFM95 not detected");
  //   delay(5000);
  //   return;
  // }
  // lora.setDeviceClass(CLASS_C);
  // lora.setDataRate(SF7BW125);
  // lora.setChannel(MULTI);
  // lora.setNwkSKey(nwkSKey);
  // lora.setAppSKey(appSKey);
  // lora.setDevAddr(devAddr);
}

void loop()
{
  while (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
    {
      displayInfo();
      sprintf(payload, "*%s*%s#", str_lat, str_lon);
    }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while (true)
      ;
  }

  // if (millis() - previousMillis > interval)
  // {
  //   previousMillis = millis();
  //   sprintf(myStr, "*%d%d*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%d*%d*%d*%d#", 69696969, 217072470, "0.0000", "0.0000", "45.3880", "0.0000", "0.0000", "0.0009", "0.0000", "0.0000", "21.0000", "0.5270", "50.0050", 810, 36150, 6960, 43110);
  //   Serial.print("Sending: ");
  //   Serial.println(myStr);
  //   lora.sendUplink(myStr, strlen(myStr), 0, 1);
  //   // Serial.println("halo");

  //   counter++;
  // }

  // lora.update();
}