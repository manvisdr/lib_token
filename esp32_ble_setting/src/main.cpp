

// const char *remote_host = "www.google.com";

// #define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// #define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// BLEServer *pServer = NULL;
// BLECharacteristic *pCharacteristic = NULL;
// bool deviceConnected = false;
// bool oldDeviceConnected = false;

// const int ledPin = 12;
// const int modeAddr = 0;
// const int wifiAddr = 10;

// int modeIdx;

// String read_String(int add);
// String getValue(String data, char separator, int index);
// void bleTask();
// void wifiTask();

// String BOT_TOKEN = "1130926242:AAHcJrGfckEEednj09igJ9Bu5QXqImfz-tg";
// String CHAT_ID = "966975362";

// #define FLASH_LED_PIN 4
// bool flashState = LOW;
// bool sendPhoto = false;
// const int led = 12;
// // ===========================
// // Enter your WiFi credentials
// // ===========================
// // const char *ssid = "MGI-MNC";
// // const char *password = "#neurixmnc#";

// int val = 0;
// int botRequestDelay = 1000;
// unsigned long lastTimeBotRan;

// WiFiClientSecure client;
// UniversalTelegramBot bot(BOT_TOKEN, client);

// Adafruit_MCP23017 mcp;

// long bot_last_check;
// int bot_check_interval = 3000;

// bool hasMoreData;
// camera_fb_t *fb = NULL;

// int PIRstate = LOW;

// bool hasMoreDataAvailable();
// byte *getNextBuffer();
// int getBufferLen();

// class MyServerCallbacks : public BLEServerCallbacks
// {
//   void onConnect(BLEServer *pServer)
//   {
//     deviceConnected = true;
//     BLEDevice::startAdvertising();
//   };

//   void onDisconnect(BLEServer *pServer)
//   {
//     deviceConnected = false;
//   }
// };

// class MyCallbacks : public BLECharacteristicCallbacks
// {
//   void onWrite(BLECharacteristic *pCharacteristic)
//   {
//     std::string value = pCharacteristic->getValue();

//     if (value.length() > 0)
//     {
//       Serial.print("Value : ");
//       Serial.println(value.c_str());
//       writeString(wifiAddr, value.c_str());
//     }
//   }

//   void writeString(int add, String data)
//   {
//     int _size = data.length();
//     for (int i = 0; i < _size; i++)
//     {
//       EEPROM.write(add + i, data[i]);
//     }
//     EEPROM.write(add + _size, '\0');
//     EEPROM.commit();
//   }
// };

// void bleTask()
// {
//   // Create the BLE Device
//   BLEDevice::init("MonTok");

//   // Create the BLE Server
//   pServer = BLEDevice::createServer();
//   pServer->setCallbacks(new MyServerCallbacks());

//   // Create the BLE Service
//   BLEService *pService = pServer->createService(SERVICE_UUID);

//   // Create a BLE Characteristic
//   pCharacteristic = pService->createCharacteristic(
//       CHARACTERISTIC_UUID,
//       BLECharacteristic::PROPERTY_READ |
//           BLECharacteristic::PROPERTY_WRITE |
//           BLECharacteristic::PROPERTY_NOTIFY |
//           BLECharacteristic::PROPERTY_INDICATE);

//   pCharacteristic->setCallbacks(new MyCallbacks());
//   // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
//   // Create a BLE Descriptor
//   pCharacteristic->addDescriptor(new BLE2902());

//   // Start the service
//   pService->start();

//   // Start advertising
//   BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
//   pAdvertising->addServiceUUID(SERVICE_UUID);
//   pAdvertising->setScanResponse(false);
//   pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
//   BLEDevice::startAdvertising();
//   Serial.println("Waiting a client connection to notify...");
// }

// void wifiTask()
// {
//   String receivedData;
//   receivedData = read_String(wifiAddr);

//   if (receivedData.length() > 0)
//   {
//     String wifiName = getValue(receivedData, ',', 0);
//     String wifiPassword = getValue(receivedData, ',', 1);

//     if (wifiName.length() > 0 && wifiPassword.length() > 0)
//     {
//       Serial.print("WifiName : ");
//       Serial.println(wifiName);

//       Serial.print("wifiPassword : ");
//       Serial.println(wifiPassword);

//       WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
//       Serial.print("Connecting to Wifi");
//       while (WiFi.status() != WL_CONNECTED)
//       {
//         Serial.print(".");
//         delay(300);
//       }
//       Serial.println();
//       Serial.print("Connected with IP: ");
//       Serial.println(WiFi.localIP());

//       Serial.print("Ping Host: ");
//       Serial.println(remote_host);

//       if (Ping.ping(remote_host))
//       {
//         Serial.println("Success!!");
//       }
//       else
//       {
//         Serial.println("ERROR!!");
//       }
//     }
//   }
// }

// String read_String(int add)
// {
//   char data[100];
//   int len = 0;
//   unsigned char k;
//   k = EEPROM.read(add);
//   while (k != '\0' && len < 500)
//   {
//     k = EEPROM.read(add + len);
//     data[len] = k;
//     len++;
//   }
//   data[len] = '\0';
//   return String(data);
// }

// String getValue(String data, char separator, int index)
// {
//   int found = 0;
//   int strIndex[] = {0, -1};
//   int maxIndex = data.length() - 1;

//   for (int i = 0; i <= maxIndex && found <= index; i++)
//   {
//     if (data.charAt(i) == separator || i == maxIndex)
//     {
//       found++;
//       strIndex[0] = strIndex[1] + 1;
//       strIndex[1] = (i == maxIndex) ? i + 1 : i;
//     }
//   }
//   return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
// }

// bool hasMoreDataAvailable()
// {
//   Serial.println("Has more daa");
//   if (hasMoreData)
//   {
//     hasMoreData = false;
//     return true;
//   }

//   return false;
// }

// byte *getNextBuffer()
// {
//   Serial.println("Next Buffer ");
//   if (fb)
//     return fb->buf;

//   return nullptr;
// }

// int getBufferLen()
// {
//   Serial.println("Buffer len");
//   if (fb)
//     return fb->len;

//   return 0;
// }

// void sendImage(String chat_id)
// {
//   Serial.println("Sending Image");
//   fb = NULL;
//   fb = esp_camera_fb_get();
//   hasMoreData = true;

//   Serial.println(fb->len);

//   bot.sendPhotoByBinary(chat_id, "image/jpeg", fb->len, hasMoreDataAvailable, nullptr, getNextBuffer, getBufferLen);

//   esp_camera_fb_return(fb);
// }

// void handleNewMessages(int numNewMessages)
// {
//   Serial.print("Handle New Messages: ");
//   Serial.println(numNewMessages);

//   for (int i = 0; i < numNewMessages; i++)
//   {
//     String chat_id = String(bot.messages[i].chat_id);
//     Serial.printf("CHAT ID MASUK : %d\n", chat_id);
//     if (chat_id != CHAT_ID)
//     {
//       bot.sendMessage(chat_id, "Unauthorized user", "");
//       continue;
//     }

//     // Print the received message
//     String text = bot.messages[i].text;
//     Serial.println(text);

//     String from_name = bot.messages[i].from_name;
//     if (text == "/start")
//     {
//       String welcome = "Welcome , " + from_name + "\n";
//       welcome += "Use the following commands to interact with the ESP32-CAM \n";
//       welcome += "/photo : takes a new photo\n";
//       welcome += "/flash : toggles flash LED \n";
//       bot.sendMessage(CHAT_ID, welcome, "");
//     }
//     if (text == "/flash")
//     {
//       flashState = !flashState;
//       digitalWrite(FLASH_LED_PIN, flashState);
//       Serial.println("Change flash LED state");
//     }
//     if (text == "/photo")
//     {
//       sendPhoto = true;
//       Serial.println("New photo request");
//     }
//   }
// }

// String sendPhotoTelegram()
// {
//   const char *myDomain = "api.telegram.org";
//   String getAll = "";
//   String getBody = "";

//   camera_fb_t *fb = NULL;
//   fb = esp_camera_fb_get();
//   if (!fb)
//   {
//     Serial.println("Camera capture failed");
//     delay(1000);
//     ESP.restart();
//     return "Camera capture failed";
//   }

//   Serial.println("Connect to " + String(myDomain));

//   if (client.connect(myDomain, 443))
//   {
//     Serial.println("Connection successful");

//     String head = "--c010blind3ngineer\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--c010blind3ngineer\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
//     String tail = "\r\n--c010blind3ngineer--\r\n";

//     uint16_t imageLen = fb->len;
//     uint16_t extraLen = head.length() + tail.length();
//     uint16_t totalLen = imageLen + extraLen;

//     client.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
//     client.println("Host: " + String(myDomain));
//     client.println("Content-Length: " + String(totalLen));
//     client.println("Content-Type: multipart/form-data; boundary=c010blind3ngineer");
//     client.println();
//     client.print(head);

//     uint8_t *fbBuf = fb->buf;
//     size_t fbLen = fb->len;
//     for (size_t n = 0; n < fbLen; n = n + 1024)
//     {
//       if (n + 1024 < fbLen)
//       {
//         client.write(fbBuf, 1024);
//         fbBuf += 1024;
//       }
//       else if (fbLen % 1024 > 0)
//       {
//         size_t remainder = fbLen % 1024;
//         client.write(fbBuf, remainder);
//       }
//     }

//     client.print(tail);

//     esp_camera_fb_return(fb);

//     int waitTime = 10000; // timeout 10 seconds
//     long startTimer = millis();
//     boolean state = false;

//     while ((startTimer + waitTime) > millis())
//     {
//       Serial.print(".");
//       delay(100);
//       while (client.available())
//       {
//         char c = client.read();
//         if (state == true)
//           getBody += String(c);
//         if (c == '\n')
//         {
//           if (getAll.length() == 0)
//             state = true;
//           getAll = "";
//         }
//         else if (c != '\r')
//           getAll += String(c);
//         startTimer = millis();
//       }
//       if (getBody.length() > 0)
//         break;
//     }
//     client.stop();
//     Serial.println(getBody);
//   }
//   else
//   {
//     getBody = "Connected to api.telegram.org failed.";
//     Serial.println("Connected to api.telegram.org failed.");
//   }
//   return getBody;
// }

// void setup()
// {
//   Serial.begin(115200);
//   Serial.setDebugOutput(true);
//   Serial.println();

//   pinMode(led, OUTPUT);
//   pinMode(FLASH_LED_PIN, OUTPUT);

//   Wire.begin(15, 14);
//   mcp.begin(&Wire);
//   mcp.pinMode(0, INPUT);

//   camera_config_t config;
//   config.ledc_channel = LEDC_CHANNEL_0;
//   config.ledc_timer = LEDC_TIMER_0;
//   config.pin_d0 = Y2_GPIO_NUM;
//   config.pin_d1 = Y3_GPIO_NUM;
//   config.pin_d2 = Y4_GPIO_NUM;
//   config.pin_d3 = Y5_GPIO_NUM;
//   config.pin_d4 = Y6_GPIO_NUM;
//   config.pin_d5 = Y7_GPIO_NUM;
//   config.pin_d6 = Y8_GPIO_NUM;
//   config.pin_d7 = Y9_GPIO_NUM;
//   config.pin_xclk = XCLK_GPIO_NUM;
//   config.pin_pclk = PCLK_GPIO_NUM;
//   config.pin_vsync = VSYNC_GPIO_NUM;
//   config.pin_href = HREF_GPIO_NUM;
//   config.pin_sscb_sda = SIOD_GPIO_NUM;
//   config.pin_sscb_scl = SIOC_GPIO_NUM;
//   config.pin_pwdn = PWDN_GPIO_NUM;
//   config.pin_reset = RESET_GPIO_NUM;
//   config.xclk_freq_hz = 20000000;
//   config.frame_size = FRAMESIZE_UXGA;
//   config.pixel_format = PIXFORMAT_JPEG; // for streaming
//   // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
//   config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
//   config.fb_location = CAMERA_FB_IN_PSRAM;
//   config.jpeg_quality = 12;
//   config.fb_count = 1;

//   // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
//   //                      for larger pre-allocated frame buffer.
//   if (config.pixel_format == PIXFORMAT_JPEG)
//   {
//     if (psramFound())
//     {
//       config.jpeg_quality = 10;
//       config.fb_count = 2;
//       config.grab_mode = CAMERA_GRAB_LATEST;
//     }
//     else
//     {
//       // Limit the frame size when PSRAM is not available
//       config.frame_size = FRAMESIZE_SVGA;
//       config.fb_location = CAMERA_FB_IN_DRAM;
//     }
//   }
//   else
//   {
//     // Best option for face detection/recognition
//     config.frame_size = FRAMESIZE_240X240;
// #if CONFIG_IDF_TARGET_ESP32S3
//     config.fb_count = 2;
// #endif
//   }

// #if defined(CAMERA_MODEL_ESP_EYE)
//   pinMode(13, INPUT_PULLUP);
//   pinMode(14, INPUT_PULLUP);
// #endif

//   // camera init
//   esp_err_t err = esp_camera_init(&config);
//   if (err != ESP_OK)
//   {
//     Serial.printf("Camera init failed with error 0x%x", err);
//     return;
//   }

//   sensor_t *s = esp_camera_sensor_get();
//   // initial sensors are flipped vertically and colors are a bit saturated
//   if (s->id.PID == OV3660_PID)
//   {
//     s->set_vflip(s, 1);       // flip it back
//     s->set_brightness(s, 1);  // up the brightness just a bit
//     s->set_saturation(s, -2); // lower the saturation
//   }
//   // drop down frame size for higher initial frame rate
//   if (config.pixel_format == PIXFORMAT_JPEG)
//   {
//     s->set_framesize(s, FRAMESIZE_QVGA);
//   }

// #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
//   s->set_vflip(s, 1);
//   s->set_hmirror(s, 1);
// #endif

// #if defined(CAMERA_MODEL_ESP32S3_EYE)
//   s->set_vflip(s, 1);
// #endif

//   // put your setup code here, to run once:
//   Serial.begin(115200);
//   pinMode(ledPin, OUTPUT);

//   if (!EEPROM.begin(EEPROM_SIZE))
//   {
//     delay(1000);
//   }

//   modeIdx = EEPROM.read(modeAddr);
//   Serial.print("modeIdx : ");
//   Serial.println(modeIdx);

//   EEPROM.write(modeAddr, modeIdx != 0 ? 0 : 1);
//   EEPROM.commit();

//   if (modeIdx != 0)
//   {
//     // BLE MODE
//     digitalWrite(ledPin, true);
//     Serial.println("BLE MODE");
//     bleTask();
//   }
//   else
//   {
//     // WIFI MODE
//     digitalWrite(ledPin, false);
//     Serial.println("WIFI MODE");
//     wifiTask();

//     client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
//     while (WiFi.status() != WL_CONNECTED)
//     {
//       delay(500);
//       Serial.print(".");
//     }
//     Serial.println("");
//     Serial.println("WiFi connected ip - ");
//     Serial.println(WiFi.localIP());
//   }

//   bot.longPoll = 60;
//   // mcp.digitalWrite(0, HIGH);
// }

// void loop()
// {
//   // val = mcp.digitalRead(0);

//   // if (val == HIGH)
//   // {
//   //   digitalWrite(led, HIGH);
//   //   if (PIRstate == LOW)
//   //   {
//   //     // we have just turned on because movement is detected
//   //     Serial.println("Motion detected!");
//   //     delay(500);
//   //     Serial.println("Sending photo to Telegram");
//   //     sendPhotoTelegram();
//   //     PIRstate = HIGH;
//   //   }
//   // }
//   // else if (sendPhoto)
//   // {
//   //   Serial.println("Preparing photo");
//   //   digitalWrite(FLASH_LED_PIN, HIGH);
//   //   Serial.println("Flash state set to HIGH");
//   //   delay(500);
//   //   sendPhotoTelegram();
//   //   sendPhoto = false;
//   //   digitalWrite(FLASH_LED_PIN, LOW);
//   //   Serial.println("Flash state set to LOW");
//   // }
//   // else if (millis() > lastTimeBotRan + botRequestDelay)
//   // {
//   //   int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//   //   while (numNewMessages)
//   //   {
//   //     Serial.println("got response");
//   //     handleNewMessages(numNewMessages);
//   //     numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//   //   }
//   //   lastTimeBotRan = millis();
//   // }
//   // else
//   // {
//   //   digitalWrite(led, LOW);
//   //   if (PIRstate == HIGH)
//   //   {
//   //     Serial.println("Motion ended!");
//   //     PIRstate = LOW;
//   //   }
//   // }
// }

#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"

#include "EEPROM.h"
#define EEPROM_SIZE 128

#include <WiFi.h>
#include <ESP32Ping.h>
#include "WiFiClientSecure.h"
#include <HTTPClient.h>
#include <Wire.h>
#include "Adafruit_MCP23017.h"
#include <ArduinoJson.h>
const char *remote_host = "www.google.com";

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

HTTPClient httpClient;
const int ledPin = 12;
const int modeAddr = 0;
const int wifiAddr = 10;

String receivedData;

String wifiName;
String wifiPassword;
const char *post_url = "http://203.194.112.238:8081/montok/log"; // Location where images are POSTED
bool internet_connected = false;

bool wifiConnected = false;

int cnt_wifi_timeout = 0;
int modeIdx;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      Serial.print("Value : ");
      Serial.println(value.c_str());
      writeString(wifiAddr, value.c_str());
      modeIdx = 0;
      EEPROM.write(modeAddr, modeIdx);
      EEPROM.commit();
      delay(1000);
      ESP.restart();
    }
  }

  void writeString(int add, String data)
  {
    int _size = data.length();
    for (int i = 0; i < _size; i++)
    {
      EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + _size, '\0');
    EEPROM.commit();
  }
};

void bleTask()
{
  // Create the BLE Device
  BLEDevice::init("MonToken");
  digitalWrite(ledPin, false);
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  pCharacteristic->setCallbacks(new MyCallbacks());
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

String read_String(int add)
{
  char data[100];
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500)
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool wifiStartTask()
{
  receivedData = read_String(wifiAddr);

  if (receivedData.length() > 0)
  {
    wifiName = getValue(receivedData, ',', 0);
    wifiPassword = getValue(receivedData, ',', 1);

    if (wifiName.length() > 0 && wifiPassword.length() > 0)
    {
      Serial.print("WifiName : ");
      Serial.println(wifiName);

      Serial.print("wifiPassword : ");
      Serial.println(wifiPassword);

      WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
      Serial.print("Connecting to Wifi");

      while (WiFi.status() != WL_CONNECTED and cnt_wifi_timeout != 10)
      {
        Serial.print(".");
        delay(500);
        cnt_wifi_timeout++;
        if (cnt_wifi_timeout == 10)
        {
          modeIdx = 0;
          EEPROM.write(modeAddr, modeIdx);
          EEPROM.commit();
          Serial.printf("CANNOT CONNECT WIFI to %s\n", wifiName);
        }
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        digitalWrite(ledPin, true);
        modeIdx = 1;
        EEPROM.write(modeAddr, modeIdx);
        EEPROM.commit();

        Serial.println();
        Serial.print("Connected with IP: ");
        Serial.println(WiFi.localIP());

        Serial.print("Ping Host: ");
        Serial.println(remote_host);

        if (Ping.ping(remote_host))
        {
          Serial.println("Success!!");
        }
        else
        {
          Serial.println("ERROR!!");
        }
      }
    }
  }
  return (WiFi.status() == WL_CONNECTED);
}

void camInit()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // init with high specs to pre-allocate larger buffers
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA; // 1600x1200
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void sendCapture()
{
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  httpClient.begin(post_url);
  httpClient.addHeader("Accept","*/*");
  httpClient.addHeader("Content-Type", "multipart/form-data");
  httpClient.addHeader("User-Agent","Thunder Client (https://www.thunderclient.com)");

  httpClient.println("Content-Length: " + (base64image.length() + image.length()));
  httpClient.println("Content-Type: image/jpeg" );
  httpClient.println();
  httpClient.write((image + base64image).c_str());

  Serial.print("[HTTP] POST...\n");
  
  int httpCode = httpClient.POST(fb->buf);
  // httpClient.sendRequest("POST", fb->buf, fb->len); 

  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = httpClient.getString();
      Serial.println(payload);
    }
  }
  else
  {
    Serial.printf("[HTTP] POST... failed, error: %s\n", httpClient.errorToString(httpCode).c_str());
  }

  httpClient.end();

  esp_camera_fb_return(fb);
}

void setup()
{
  cnt_wifi_timeout = 0;
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    delay(1000);
  }
  camInit();

  modeIdx = EEPROM.read(modeAddr);
  Serial.print("MODE ");
  Serial.println(EEPROM.read(modeAddr));

  wifiStartTask();
  bleTask();

  sendCapture();
}

void loop()
{
}