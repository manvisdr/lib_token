#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>

SoftwareSerial swSer;

#define csipOne 15 // putih receiver modbus
#define csResIpOne 34
#define csipTwo 12 // ungu tranmitter to modem
#define TCPPORT 23
#define MAXCLIENTS 3
#define BAUDRATE 9600

// LAN CONF
byte macOne[] = {0x90, 0xA2, 0xDA, 0x0f, 0x15, 0x81};
byte macTwo[] = {0x90, 0xA2, 0xDA, 0x0f, 0x15, 0x4C};

IPAddress ipOne(192, 168, 0, 106); // ethernet 1
IPAddress ipTwo(192, 168, 0, 105); // ethernet 2

EthernetServer serverOne(80); // server ethernet 1
// EthernetServer serverTwo(80); // server ethernet 2

String c = "";
/*
void startEthOne()
{
  Serial.print(F("Starting One..."));
  Ethernet.init(csipOne);
  Ethernet.begin(macOne, ipOne);
  Serial.println(Ethernet.localIP());
  serverOne.begin();
}

void startEthTwo()
{
  Serial.print(F("Starting Two..."));
  Ethernet.init(csipTwo);
  Ethernet.begin(macTwo, ipTwo);
  Serial.println(Ethernet.localIP());
  serverTwo.begin();
}

void lan_one_to_two()
{
  // ethernet one read //
  Ethernet.init(csipOne);
  EthernetClient client = serverOne.available();

  if (client)
  {
    if (client.available() > 0)
    {
      char thisChar = (char)client.read();
      c += thisChar;

      if (thisChar == '*')
      {
        client.stop();

        // ethernet two send
        Ethernet.init(csipTwo);
        EthernetClient client = serverTwo.available();

        char charSend[] = "acknowledged";
        serverTwo.write(charSend);

        // for (int i = 0; i < c.length()-1; i++ ) {
        //     Serial.println('Lan Two Connect');
        //         serverTwo.write((byte)c[i]);
        //   }
        // if (client) {  //starts client connection, checks for connection
        //   Serial.println('Lan Two Connect');
        //   for (int i = 0; i < c.length()-1; i++ ) {
        //         serverTwo.write((byte)c[i]);
        //   }
        // }
        client.stop();
        Serial.print(c);
        // c = "";
        // Ethernet.init(5);
      }
    }
  }
}

void WizOffOne()
{
  Serial.print("OFF Ethernet Board...  ");
  digitalWrite(csResIpOne, LOW);
  delay(350);
  Serial.print("Done.\n");
}

void WizOnOne()
{
  Serial.print("ON Ethernet Board...  ");
  digitalWrite(csResIpOne, HIGH);
  delay(350);
  Serial.print("Done.\n");
}
*/
void setup()
{
  Serial.begin(9600);
  swSer.begin(9600, SWSERIAL_8N1, 22, 21, false, 256);

  Ethernet.init(csipOne);
  Ethernet.begin(macOne, ipOne);
  serverOne.begin();

  // pinMode(csResIpOne, OUTPUT);
  // pinMode(csipOne, OUTPUT);
  // digitalWrite(csipOne, HIGH);
  // startEthOne();

  // pinMode(csipTwo, OUTPUT);
  // digitalWrite(csipTwo, HIGH);
  // startEthTwo();

  Serial.println(F("Starting ethernet..."));
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("Ethernet cable is not connected.");
  }

  delay(2000);
  Serial.print("Chat server address:");
  Serial.println(Ethernet.localIP());
}

void loop()
{
  delay(1000);

  int randomData = random(0, 1000);
  serverOne.print("ETH ONE Sent from Arduino : ");
  serverOne.println(randomData);

  // lan_one_to_two();
}

// -------------------------------------------------------------------------
