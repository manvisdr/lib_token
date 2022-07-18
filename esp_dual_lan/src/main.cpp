#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ModbusClientTCPasync.h>

#define csipOne 17  //putih receiver modbus
#define csipTwo 5   //ungu tranmitter to modem

byte macOne[] = {0x90, 0xA2, 0xDA, 0x0f, 0x15, 0x81};
byte macTwo[] = {0x90, 0xA2, 0xDA, 0x0f, 0x15, 0x4C};

IPAddress ipOne(192, 168, 0, 106);  //ethernet 1
IPAddress ipTwo(192, 168, 0, 105);  //ethernet 2

EthernetClient modbusEth;           //client ethernet 1 sebagai ethernet client utk modbus
// EthernetServer serverOne(80);       //server ethernet 1
EthernetServer serverTwo(80);       //server ethernet 2

IPAddress ipSlaveModbus = {192, 168, 2, 5};    // IP address of modbus server
uint16_t portModbus = 502;                // port of modbus server
ModbusClientTCPasync modbusTCP(ipModbus, portModbus);

boolean currentLineIsBlank = true;
String c = "";

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

void handleData(ModbusMessage response, uint32_t token) 
{
  Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
  for (auto& byte : response) {
    Serial.printf("%02X ", byte);
  }
  Serial.println("");
}

void handleError(Error error, uint32_t token) 
{
  ModbusError me(error);
  Serial.printf("Error response: %02X - %s token: %d\n", (int)me, (const char *)me, token);
}

void lan_one_to_two()
{
  // ethernet one read //
  Ethernet.init(csipOne);
  EthernetClient client = serverOne.available();
  currentLineIsBlank = true;

  if (client) {
    if (client.available() > 0) {
      char thisChar = (char)client.read();
      c += thisChar;

      if (thisChar == '*')
      {
        client.stop();

        //ethernet two send
        Ethernet.init(csipTwo);
        EthernetClient client = serverTwo.available();
        currentLineIsBlank = true;

        char charSend [] = "acknowledged";
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

void setup()
{
  Serial.begin(115200);

  pinMode(csipOne,OUTPUT);
  digitalWrite(csipOne,HIGH);

  pinMode(csipTwo,OUTPUT);
  digitalWrite(csipTwo,HIGH);

  modbusTCP.onDataHandler(&handleData);
  modbusTCP.onErrorHandler(&handleError);
  modbusTCP.setTimeout(10000);
  modbusTCP.setIdleTimeout(60000);

  startEthOne();
  startEthTwo();

  Serial.println(F("Starting ethernet..."));
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
  

  delay(2000);
  Serial.print("Chat server address:");
  Serial.println(Ethernet.localIP());
}

void loop()
{
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    Serial.printf("sending request with token %d\n", (uint32_t)lastMillis);
    Error err;
    err = modbusTCP.addRequest((uint32_t)lastMillis, 1, READ_HOLD_REGISTER, 40000, 10);
    if (err != SUCCESS) {
      ModbusError e(err);
      Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    }
  }
  lan_one_to_two();
}

// -------------------------------------------------------------------------
