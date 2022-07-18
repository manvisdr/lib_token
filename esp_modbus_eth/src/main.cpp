#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ModbusClientTCP.h>

#define csipOne 17                                          //putih receiver modbus
IPAddress lIP;                                              // IP address after Ethernet.begin()
byte macOne[] = {0x90, 0xA2, 0xDA, 0x0f, 0x15, 0x81};
IPAddress ipOne(192, 168, 0, 106);                          //ethernet 1
EthernetClient modbusEth;                                   //client ethernet 1 sebagai ethernet client utk modbus
IPAddress ipSlaveModbus = {192, 168, 0, 101};                 // IP address of modbus server
uint16_t portModbus = 502;                                  // port of modbus server
ModbusClientTCP modbusTCP(modbusEth);
String c = "";

void handleData(ModbusMessage response, uint32_t token) 
{
  uint16_t addr, words;
  response.get(2, addr);
  response.get(4, words);
  Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", response.getServerID(), response.getFunctionCode(), token, response.size());
  for (auto& byte : response) {
    Serial.printf("%02X ", byte);
  }
  Serial.println("");
  Serial.print(addr);Serial.print(" ");Serial.println(words);
}

void handleError(Error error, uint32_t token) 
{
  ModbusError me(error);
  Serial.printf("Error response: %02X - %s token: %d\n", (int)me, (const char *)me, token);
}

void WizReset() {
  Serial.print("Resetting Wiz W5500 Ethernet Board...  ");
  pinMode(csipOne, OUTPUT);
  digitalWrite(csipOne, HIGH);
  delay(250);
  digitalWrite(csipOne, LOW);
  delay(50);
  digitalWrite(csipOne, HIGH);
  delay(350);
  Serial.print("Done.\n");
}

void setup() {
// Init Serial monitor
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("__ OK __");

  Ethernet.init(csipOne);
  WizReset();// Hard boot W5500 module
  Ethernet.begin(macOne,ipOne);
  // start the Ethernet connection:
  // if (Ethernet.begin(macOne) == 0) {
  //   Serial.print("Failed to configure Ethernet using DHCP\n");
  //   if (Ethernet.hardwareStatus() == EthernetNoHardware) {
  //     Serial.print("Ethernet shield was not found.  Sorry, can't run without hardware. :(\n");
  //   } else {
  //     if (Ethernet.linkStatus() == LinkOFF) {
  //       Serial.print("Ethernet cable is not connected.\n");
  //     }
  //   }
  //   while (1) {}  // Do nothing any more
  // }

  // print local IP address:
  lIP = Ethernet.localIP();
  Serial.printf("My IP address: %u.%u.%u.%u\n", lIP[0], lIP[1], lIP[2], lIP[3]);

  modbusTCP.onDataHandler(&handleData);
  modbusTCP.onErrorHandler(&handleError);
  modbusTCP.setTimeout(2000, 200);

  modbusTCP.begin();
  modbusTCP.setTarget(ipSlaveModbus, portModbus);
 
}

void loop() {
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    Serial.printf("sending request with token %d\n", (uint32_t)lastMillis);
    Error err;
    err = modbusTCP.addRequest((uint32_t)lastMillis, 1, READ_HOLD_REGISTER, 40000, 10);
    if (err!=SUCCESS) {
      ModbusError e(err);
      Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
  }
  }
}
