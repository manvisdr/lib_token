#include <Arduino.h>
#include <SPI.h>
#include <lorawan.h>
#include <EthernetSPI2.h>

#define csEthOne 15 // putih receiver modbus
#define csResIpOne 27
// LORA CONF
// ABP Credentials
const char *devAddr = "260DB31D";
const char *nwkSKey = "45789AEC3AA3D97D22602D0CDDC5BBA8";
const char *appSKey = "3868C323B2A6AD96AB8BAFB7AB3B002C";

const unsigned long interval = 1000; // 10 s interval to send message
unsigned long previousMillis = 0;    // will store last time message sent
unsigned int counter = 0;            // message counter

char myStr[50];
char outStr[255];
byte recvStatus = 0;

const sRFM_pins RFM_pins = {
    .CS = 5,
    .RST = 26,
    .DIO0 = 2,
    .DIO1 = 4,
};

// ETHERNET CONF
byte macEthOne[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

IPAddress ipEthOne(192, 168, 0, 106); // ethernet 1

// Membuka akses port 23 untuk protokol TCP
EthernetServer server(80);



void setup()
{
  Serial.begin(115200);

  if (!lora.init())
  {
    Serial.println("RFM95 not detected");
    delay(5000);
    return;
  }

  lora.setDeviceClass(CLASS_C);
  lora.setDataRate(SF10BW125);
  lora.setChannel(MULTI);
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);

   Ethernet.init(csEthOne);
   Ethernet.begin(macEthOne, ipEthOne);
   server.begin();
   Serial.println(F("Starting ethernet..."));
   if (Ethernet.hardwareStatus() == EthernetNoHardware)
   {
     Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
     while (true)
     {
       delay(1);
     }
   }
   if (Ethernet.linkStatus() == LinkOFF)
   {
     Serial.println("Ethernet cable is not connected.");
   }
   Serial.println(Ethernet.localIP());
}

void loop()
{

  if (millis() - previousMillis > interval)
  {
    previousMillis = millis();
    int coba = 1;
    sprintf(myStr, "coba :%d", coba);

    Serial.print("Sending: ");
    Serial.print(myStr);
    lora.sendUplink(myStr, strlen(myStr), 0, 1);
    Serial.print(F(" w/ RSSI "));
    Serial.println(lora.getRssi());
    counter++;
  }
  lora.update();

  delay(1000);

   int randomData = random(0, 1000);
   server.print("Sent from Arduino : ");
   server.println(randomData);
   delay(1000);
}