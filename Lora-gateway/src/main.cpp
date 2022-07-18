#include <Arduino.h>
#include <lorawan.h>
#include <SPI.h>
#include <WiFi.h>
#include <ModbusClientTCPasync.h>
//ABP Credentials
const char *devAddr = "260DB31D";
const char *nwkSKey = "45789AEC3AA3D97D22602D0CDDC5BBA8";
const char *appSKey = "3868C323B2A6AD96AB8BAFB7AB3B002C";


char ssid[] = "MGI-MNC";                     // SSID and ...
char pass[] = "#neurixmnc#";                     // password for the WiFi network used
IPAddress ip = {192, 168, 2, 5};          // IP address of modbus server
uint16_t port = 502;                      // port of modbus server

const unsigned long interval = 1000;    // 10 s interval to send message
unsigned long previousMillis = 0;  // will store last time message sent
unsigned int counter = 0;     // message counter

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

ModbusClientTCPasync MB(ip, port);

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

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("__ OK __");

// Connect to WiFi
  WiFi.begin(ssid, pass);
  delay(200);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(". ");
    delay(1000);
  }
  IPAddress wIP = WiFi.localIP();
  Serial.printf("WIFi IP address: %u.%u.%u.%u\n", wIP[0], wIP[1], wIP[2], wIP[3]);

  if (!lora.init()) {
    Serial.println("RFM95 not detected");
    delay(5000);
    return;
  }

  // Set LoRaWAN Class change CLASS_A or CLASS_C
  lora.setDeviceClass(CLASS_C);

  // Set Data Rate
  lora.setDataRate(SF10BW125);

  // set channel to random
  lora.setChannel(MULTI);

  // Put ABP Key and DevAddress here
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);

  MB.onDataHandler(&handleData);
  MB.onErrorHandler(&handleError);
  MB.setTimeout(10000);
  MB.setIdleTimeout(60000);

}

void loop() {
  // //float t = dht.readTemperature();
  // if (millis() - previousMillis > interval) {
  //   previousMillis = millis();
  //   int coba=1;
  // //    String coba = "Suhu :" + String(float(mlx.readObjectTempC()))
  // //    println(coba);
  //   sprintf(myStr, "coba :%d", coba);
  //   //    Serial.print(F("%  Temperature: "));
  //   //    Serial.println(t);
  //   Serial.print("Sending: ");
  //   Serial.println(myStr);
  //   //
  //   lora.sendUplink(myStr, strlen(myStr), 0, 1);
  //           //Serial.println("halo");
  //   counter++;
  // }
  // lora.update();

  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();

    // Create request for
    // (Fill in your data here!)
    // - server ID = 1
    // - function code = 0x03 (read holding register)
    // - start address to read = word 10
    // - number of words to read = 4
    // - token to match the response with the request. We take the current millis() value for it.
    //
    // If something is missing or wrong with the call parameters, we will immediately get an error code 
    // and the request will not be issued
    Serial.printf("sending request with token %d\n", (uint32_t)lastMillis);
    Error err;
    err = MB.addRequest((uint32_t)lastMillis, 1, READ_HOLD_REGISTER, 40000, 10);
    if (err != SUCCESS) {
      ModbusError e(err);
      Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
    }
    // Else the request is processed in the background task and the onData/onError handler functions will get the result.
    //
    // The output on the Serial Monitor will be (depending on your WiFi and Modbus the data will be different):
    //     __ OK __
    //     . WIFi IP address: 192.168.178.74
    //     Response: serverID=20, FC=3, Token=0000056C, length=11:
    //     14 03 04 01 F6 FF FF FF 00 C0 A8
  }

}

