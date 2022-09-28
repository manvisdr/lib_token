#include <Arduino.h>
// #define TIME_METER
#define TIME_RTC
#ifdef TIME_RTC
#include <DS1307RTC.h>
#endif
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <SPI.h>
#include "EDMICmdLine.h"
#include <lorawan.h>

#define TYPE_MK10e
#define TYPE_SEND_LORA

#if defined(TYPE_SEND_LORA)
#define csLora 5 // PIN CS LORA
const sRFM_pins RFM_pins = {
    .CS = 5,
    .RST = 26,
    .DIO0 = 25,
    .DIO1 = 32,
}; // DEF PIN LORA

#define MODE_LORA 0 // LORA
#endif
// DEF PIN
#define RXD2 16         // PIN RX2
#define TXD2 17         // PIN TX2
#define PIN_LED_PROC 27 // PIN RX2
#define PIN_LED_WARN 15 // PIN TX2

EdmiCMDReader edmiread(Serial2, RXD2, TXD2);

//  Var CONFIG DEFAULT
String customerID = "54565456";
String kwhID;
String kwhType = "MK10E";
const String channelId = "monKWH/";
const char *devAddr = "260D6ECC";
const char *nwkSKey = "1FD053B476BF4F732DC8CF5E565538B9";
const char *appSKey = "2EE8E2940E6799AD78F9F4C6DA8C53D1";
const unsigned int interval = 5000;
const unsigned int interval_cek_konek = 2000;
const unsigned int interval_record = 20000;
long previousMillis = 0;
long previousMillis1 = 0;

int lastMinutes;

// VAR MODE KIRIM
bool kwhReadReady = false;
bool kwhSend = false;
char myStr[255];

bool loraConf()
{
  lora.setDeviceClass(CLASS_C);
  lora.setDataRate(SF7BW125);
  lora.setChannel(MULTI);
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);

  return true;
}

void backgroundTask(void)
{
  edmiread.keepAlive();
}

void blinkWarning(int times)
{
  if (times == 0)
    digitalWrite(PIN_LED_WARN, 0);
  else
  {
    for (int x = 0; x <= times; x++)
    {
      static long prevMill = 0;
      if (((long)millis() - prevMill) >= 500)
      {
        prevMill = millis();
        digitalWrite(PIN_LED_WARN, !digitalRead(PIN_LED_WARN));
      }
    }
  }
}

void blinkProcces(int times)
{
  for (int x = 0; x <= times; x++)
  {
    digitalWrite(PIN_LED_PROC, !digitalRead(PIN_LED_PROC));
    delay(50);
    digitalWrite(PIN_LED_PROC, !digitalRead(PIN_LED_PROC));
    delay(50);
  }
}

void BackgroundDelay()
{
  blinkProcces(3);
  kwhReadReady = true;
  edmiread.acknowledge();
  Serial.printf("TIME TO READ\n");
}

void BackgroundSend()
{
  blinkProcces(5);
  Serial.println(myStr);
  Serial.println("SEND WITH LORA");
  lora.sendUplink(myStr, strlen(myStr), 0, 1);
  lora.update();
}

void printDigits(int digits)
{
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void digitalClockDisplay()
{

  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.println();
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(PIN_LED_PROC, OUTPUT);
  pinMode(PIN_LED_WARN, OUTPUT);
  digitalWrite(PIN_LED_PROC, LOW);
#if defined TIME_RTC
  setSyncProvider(RTC.get); //******************************************
#elif defined TIME_METER
  edmiread.read_time();
  setTime(
      edmiread.timeEdmi.hour,
      edmiread.timeEdmi.minute,
      edmiread.timeEdmi.seconds,
      edmiread.timeEdmi.day,
      edmiread.timeEdmi.month,
      edmiread.timeEdmi.year);
#endif

  if (!lora.init())
  {
    Serial.println("RFM95 not detected");
  }
  loraConf();

  Alarm.timerRepeat(60, BackgroundDelay); // timer for every 15 seconds
  // Alarm.timerRepeat(35, BackgroundSend);  // timer for every 15 seconds
  if (kwhID.length() == 0)
  {
    kwhID = edmiread.serialNumber();
    Serial.println(kwhID);
  }

} // setup

void loop()
{
  if (kwhID.length() == 0)
  {
    kwhID = edmiread.serialNumber();
    Serial.println(kwhID);
  }
  backgroundTask();

  edmiread.read_looping();
  EdmiCMDReader::Status status = edmiread.status();
  Serial.printf("%s\n", kwhReadReady ? "true" : "false");
  if (status == EdmiCMDReader::Status::Ready and kwhReadReady)
  {
    blinkWarning(LOW);
    blinkProcces(2);
    Serial.println("STEP_START");
    edmiread.step_start();
  }
  else if (status == EdmiCMDReader::Status::Finish)
  {
    sprintf(myStr, "~*%d*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s*%s#",
            3,
            customerID,
            kwhID,
            String(edmiread.voltR()*1000, 4).c_str(),
            String(edmiread.voltS()*1000, 4).c_str(),
            String(edmiread.voltT()*1000, 4).c_str(),
            String(edmiread.currentR(), 4).c_str(),
            String(edmiread.currentS(), 4).c_str(),
            String(edmiread.currentT(), 4).c_str(),
            String(edmiread.wattR() / 10.0, 4).c_str(),
            String(edmiread.wattS() / 10.0, 4).c_str(),
            String(edmiread.wattT() / 10.0, 4).c_str(),
            String(edmiread.pf(), 4).c_str(),
            String(edmiread.frequency(), 4).c_str(),
            String(edmiread.kVarh() / 1000.0, 2).c_str(),
            String(edmiread.kwhLWBP() / 1000.0, 2).c_str(),
            String(edmiread.kwhWBP() / 1000.0, 2).c_str(),
            String(edmiread.kwhTotal() / 1000.0, 2).c_str());

    Serial.println(myStr);
    Serial.println("SEND WITH LORA");
    lora.sendUplink(myStr, strlen(myStr), 1, 1);
    lora.update();
    kwhReadReady = false;
    // digitalWrite(PIN_LED_PROC, LOW);
  }
  else if (status != EdmiCMDReader::Status::Busy)
  {
    Serial.println("BUSY");
    blinkWarning(HIGH);
  }
  // lora.update();
  digitalClockDisplay();
  Alarm.delay(1000); // wait one second between clock display
} // loop