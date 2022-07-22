#include <lorawan.h>

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

void setup()
{
  // Setup loraid access
  Serial.begin(115200);
  //  dht.begin();
  Serial.println("Start..");
  //  mlx.begin();
  // dht.begin();
  if (!lora.init())
  {
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
}

void loop()
{
  // float t = dht.readTemperature();
  if (millis() - previousMillis > interval)
  {
    previousMillis = millis();
    //

    //    Serial.print(mlx.readObjectTempC());
    //    Serial.println();
    int coba = random(0, 1000);
    //    String coba = "Suhu :" + String(float(mlx.readObjectTempC()))
    //    println(coba);
    sprintf(myStr, "coba :%d", coba);
    //    Serial.print(F("%  Temperature: "));
    //    Serial.println(t);
    Serial.print("Sending: ");
    Serial.println(myStr);

    //
    lora.sendUplink(myStr, strlen(myStr), 0, 1);
    // Serial.println("halo");

    counter++;
  }

  lora.update();
}