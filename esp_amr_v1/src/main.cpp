#include <Arduino.h>
#include <SPI.h>
#include <lorawan.h>
#include <Ethernet.h>

#define csLora 5
#define csEthOne 15 // putih receiver modbus

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
    .RST = 14,
    .DIO0 = 2,
    .DIO1 = 4,
    // .DIO2 = -1,
    // .DIO5 = -1,
};

// ETHERNET CONF
byte macEthOne[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

IPAddress ipEthOne(192, 168, 0, 106); // ethernet 1

// Membuka akses port 23 untuk protokol TCP
EthernetServer server(80);

LoRaWANClass loraa;

void setup()
{
  Serial.begin(9600);
  // pinMode(csLora, OUTPUT);
  // pinMode(csEthOne, OUTPUT);

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
  // digitalWrite(csLora, LOW);

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
  // Mengirim data ke client yang telah terkoneksi
  // digitalWrite(csEthOne, HIGH);
  int randomData = random(0, 1000);
  server.print("Sent from Arduino : ");
  server.println(randomData);
  delay(1000);

  // digitalWrite(csEthOne, LOW);
  // digitalWrite(csLora, HIGH);

  if (millis() - previousMillis > interval)
  {
    previousMillis = millis();
    int coba = 1;
    sprintf(myStr, "coba :%d", randomData);

    Serial.print("Sending: ");
    Serial.println(myStr);
    lora.sendUplink(myStr, strlen(myStr), 0, 1);
    counter++;
  }
  lora.update();

  delay(1000);
  // digitalWrite(csLora, LOW);
}