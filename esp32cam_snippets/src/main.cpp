#include <Arduino.h>
#include <WiFi.h>
#include "ping.h"
#include <ESP32Ping.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFiClient.h>
#include <esp_camera.h>
#include <TimeAlarms.h>

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define SPIFFS
#define SOUND_TIMEOUT 3000
// the setup function runs once when you press reset or power the board
const char ssid[] = "MGI-MNC";         //  your network SSID (name)
const char password[] = "#neurixmnc#"; // your network password

// const char *ssid = "NEURIXII";
// const char *password = "#neurix123#";

#define FORMAT_LITTLEFS_IF_FAILED true
String host = "203.194.112.238";                    // change to your webserver ip or name
String url = "/topup/response/saldo/?token=696969"; // change to url of your http post html file
// const String SERVER = "203.194.112.238";
const String serverEndpoint = "/topup/response/nominal/?"; // Needs to match upload server endpoint
const String imgNominal = "\"imgNominal\"";                // Needs to match upload server keyName

const String SERVER = "203.194.112.238";
// const String SERVER = "192.168.0.190";

int PORT = 3300;
#define BOUNDARY "--------------------------133747188241686651551404"
#define TIMEOUT 20000
WiFiClient client;
String idDevice = "009";
char *_status = "success";
char *_pelanggan = "01";
char *_sn = "2021";
char *_token = "241241414";
IPAddress mqttServer(203, 194, 112, 238);

void CameraInit()
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
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_VGA; // 1600x1200
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_VGA;
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
  else
    Serial.printf("CameraInit()...Succesfull\n", err);
}
String sendPhoto(char *status, char *sn, char *token);

void pingServer()
{
  if (Ping.ping(mqttServer), 2)
    Serial.println("SUCCESS");
  else
    Serial.println("FAILED");
}

void pingRun(void *parameter)
{
  for (;;)
  {
    Serial.print("Main Ping: Executing on core ");
    Serial.println(xPortGetCoreID());
    pingServer();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
}

String sendNotifSaldo(char *sn);

void setup()
{

  Serial.begin(115200);
  Serial.print("Connecting t ");
  Serial.println(ssid);
  CameraInit();

  if (!LittleFS.begin(false /* false: Do not format if mount failed */))
  {
    Serial.println("Failed to mount LittleFS");
  }
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  pinMode(17, INPUT_PULLUP);

  // Alarm.timerRepeat(15, pingServer);
  // xTaskCreatePinnedToCore(
  //     pingRun,
  //     "pingServer",     // Task name
  //     5000,             // Stack size (bytes)
  //     NULL,             // Parameter
  //     tskIDLE_PRIORITY, // Task priority
  //     NULL,             // Task handle
  //     0);

  // sendPhoto("success", "2021", "241241414");
  Serial.println(sendNotifSaldo("007"));
}

bool CheckTimeout(unsigned long *timeNow, unsigned long *timeStart)
{
  if (*timeNow - *timeStart >= SOUND_TIMEOUT)
    return true;
  else
    return false;

} // CheckTimeout

void wifisendfile(String filename, String Sendhost, String Sendurl)
{
  // prepare httpclient
  Serial.println("Starting connection to server...");
  Serial.println(Sendhost);

  delay(1000);
  // start http sending
  if (client.connect(Sendhost.c_str(), 3300))
  {
    // test sd card
    if (!LittleFS.begin())
    {
      Serial.println("SD Card Mount Failed");
      return;
    }
    else
    {
      Serial.println("SD Card OK...");
    }
    // open file
    File myFile;
    myFile = LittleFS.open(filename); // change to your file name
    int filesize = myFile.size();
    Serial.print("filesize=");
    Serial.println(filesize);
    String fileName = myFile.name();
    String fileSize = String(myFile.size());

    Serial.println("reading file");
    if (myFile)
    {
      String boundary = "CustomizBoundarye----";
      String contentType = "image/jpeg"; // change to your file type

      // prepare http post data(generally, you dont need to change anything here)
      String postHeader = "POST " + Sendurl + " HTTP/1.1\r\n";
      postHeader += "Host: " + Sendhost + ":3300 \r\n";
      postHeader += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";
      postHeader += "Accept-Charset: utf-8;\r\n";
      String keyHeader = "--" + boundary + "\r\n";
      keyHeader += "Content-Disposition: form-data; name=\"key\"\r\n\r\n";
      String requestHead = "--" + boundary + "\r\n";
      requestHead += "Content-Disposition: form-data; name=\"imgSaldo\"; filename=\"" + fileName + "\"\r\n";
      requestHead += "Content-Type: " + contentType + "\r\n\r\n";
      // post tail
      String tail = "\r\n--" + boundary + "--\r\n\r\n";
      // content length
      int contentLength = keyHeader.length() + requestHead.length() + myFile.size() + tail.length();
      postHeader += "Content-Length: " + String(contentLength, DEC) + "\n\n";

      // send post header
      char charBuf0[postHeader.length() + 1];
      postHeader.toCharArray(charBuf0, postHeader.length() + 1);
      client.write(charBuf0);
      Serial.print("send post header=");
      Serial.println(charBuf0);

      // send key header
      // char charBufKey[keyHeader.length() + 1];
      // keyHeader.toCharArray(charBufKey, keyHeader.length() + 1);
      // client.write(charBufKey);
      // Serial.print("send key header=");
      // Serial.println(charBufKey);

      // send request buffer
      char charBuf1[requestHead.length() + 1];
      requestHead.toCharArray(charBuf1, requestHead.length() + 1);
      client.write(charBuf1);
      Serial.print("send request buffer=");
      Serial.println(charBuf1);

      // create file buffer
      const int bufSize = 4096;
      byte clientBuf[bufSize];
      int clientCount = 0;

      // send myFile:
      Serial.println("Send file");
      while (myFile.available())
      {
        clientBuf[clientCount] = myFile.read();
        clientCount++;
        if (clientCount > (bufSize - 1))
        {
          client.write((const uint8_t *)clientBuf, bufSize);
          client.flush();
          Serial.write((const uint8_t *)clientBuf, bufSize);
          clientCount = 0;
        }
      }
      if (clientCount > 0)
      {
        client.write((const uint8_t *)clientBuf, clientCount);
        client.flush();
        Serial.write((const uint8_t *)clientBuf, clientCount);
      }

      // send tail
      char charBuf3[tail.length() + 1];
      tail.toCharArray(charBuf3, tail.length() + 1);
      client.write(charBuf3);
      Serial.println("send tail");
      // Serial.print(charBuf3);
    }
  }
  else
  {
    Serial.println("Connecting to server error");
  }
  // print response
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 10000)
    {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  while (client.available())
  {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  // myFile.close();
  Serial.println("closing connection");
  client.stop();
}
bool send = false;

String body_Saldo()
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  data += F("Content-Disposition: form-data; name=\"imgSaldo\"; filename=\"picture.jpg\"\r\n");
  data += F("Content-Type: image/jpeg\r\n");
  data += F("\r\n");

  return (data);
}

String headerSaldo(char *token, size_t length)
{
  String tokens = String(token);
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/saldo/?token=" + tokens + " HTTP/1.1\r\n";
  data += "cache-control: no-cache\r\n";
  data += "Content-Type: multipart/form-data; boundary=";
  data += BOUNDARY;
  data += "\r\n";
  data += "User-Agent: PostmanRuntime/6.4.1\r\n";
  data += "Accept: */*\r\n";
  data += "Host: ";
  data += SERVER;
  data += "\r\n";
  data += "accept-encoding: gzip, deflate\r\n";
  data += "Connection: keep-alive\r\n";
  data += "content-length: ";
  data += String(length);
  data += "\r\n";
  data += "\r\n";
  return (data);
}

String sendImageSaldo(char *token, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  String bodySaldo = body_Saldo();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySaldo.length() + size_pic + bodyEnd.length();
  String headerTxt = headerSaldo(token, allLen);

  if (!client.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  client.print(headerTxt + bodySaldo);
  Serial.print(headerTxt + bodySaldo);
  client.write(data_pic, size_pic);

  client.print("\r\n" + bodyEnd);
  Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (client.available())
    {
      serverRes = client.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  return serverRes;
}

String sendImageSaldo(char *filename, char *token)
{
  String serverRes;
  String bodySaldo = body_Saldo();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  File myFile;
  myFile = LittleFS.open(filename); // change to your file name
  size_t filesize = myFile.size();

  size_t allLen = bodySaldo.length() + filesize + bodyEnd.length();
  String headerTxt = headerSaldo(token, allLen);

  if (!client.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  client.print(headerTxt + bodySaldo);
  Serial.print(headerTxt + bodySaldo);
  while (myFile.available())
    client.write(myFile.read());

  client.print("\r\n" + bodyEnd);
  Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (client.available())
    {
      serverRes = client.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  return serverRes;
}

String body_Nominal()
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  data += F("Content-Disposition: form-data; name=\"imgNominal\"; filename=\"picture.jpg\"\r\n");
  data += F("Content-Type: image/jpeg\r\n");
  data += F("\r\n");

  return (data);
}

String headerNominal(size_t length)
{
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/nominal/?status=success&sn_device=" + String(idDevice) + " HTTP/1.1\r\n";
  data += "cache-control: no-cache\r\n";
  data += "Content-Type: multipart/form-data; boundary=";
  data += BOUNDARY;
  data += "\r\n";
  data += "User-Agent: PostmanRuntime/6.4.1\r\n";
  data += "Accept: */*\r\n";
  data += "Host: ";
  data += SERVER;
  data += "\r\n";
  data += "Connection: keep-alive\r\n";
  data += "content-length: ";
  data += String(length);
  data += "\r\n";
  data += "\r\n";
  return (data);
}

String sendImageNominal(uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  String bodyNominal = body_Nominal();

  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodyNominal.length() + size_pic + bodyEnd.length();
  String headerTxt = headerNominal(allLen);

  if (!client.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  client.print(headerTxt + bodyNominal);
  Serial.print(headerTxt + bodyNominal);
  client.write(data_pic, size_pic);

  client.print("\r\n" + bodyEnd);
  Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (client.available())
    {
      serverRes = client.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  return serverRes;
}

String sendImageNominal(char *filename)
{
  String serverRes;
  String bodyNominal = body_Nominal();
  File myFile;
  myFile = LittleFS.open(filename); // change to your file name
  size_t filesize = myFile.size();

  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodyNominal.length() + filesize + bodyEnd.length();
  String headerTxt = headerNominal(allLen);

  if (!client.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  client.print(headerTxt + bodyNominal);
  Serial.print(headerTxt + bodyNominal);
  while (myFile.available())
    client.write(myFile.read());

  client.print("\r\n" + bodyEnd);
  Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (client.available())
    {
      serverRes = client.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  return serverRes;
  myFile.close();
}

String sendPhoto(char *status, char *sn, char *token)
{
  String getAll;
  String getBody;

  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Connecting to server: " + SERVER);

  if (client.connect(SERVER.c_str(), PORT))
  {
    Serial.println("Connection successful!");
    // http://203.194.112.238:3300/topup/response/?status=success&sn_device=5253252&token=33333

    // String headSaldo = "--MGI\r\nContent-Disposition: form-data; name=" + imgSaldo + "; filename=" + "\"imgSaldo.jpg\"" + "\r\nContent-Type: image/jpeg\r\n\r\n";
    String headNominal = "--MGI\r\nContent-Disposition: form-data; name=" + imgNominal + "; filename=" + "\"imgNominal.jpg\"" + "\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--MGI--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = headNominal.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    client.println("POST " + serverEndpoint + "status=" + status + "&sn_device=" + sn + " HTTP/1.1");
    Serial.println("POST " + serverEndpoint + "status=" + status + "&sn_device=" + sn + " HTTP/1.1");
    client.println("Host: " + SERVER);
    Serial.println("Host: " + SERVER);
    client.println("Content-Length: " + String(totalLen));
    Serial.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=MGI");
    Serial.println("Content-Type: multipart/form-data; boundary=MGI");
    client.println();
    Serial.println();

    client.print(headNominal);
    Serial.print(headNominal);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024)
    {
      if (n + 1024 < fbLen)
      {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0)
      {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }

    // client.println();
    // Serial.println();

    // client.print(headSaldo);
    // Serial.print(headSaldo);

    // for (size_t n = 0; n < fbLen; n = n + 1024) {
    //   if (n + 1024 < fbLen) {
    //     client.write(fbBuf, 1024);
    //     fbBuf += 1024;
    //   } else if (fbLen % 1024 > 0) {
    //     size_t remainder = fbLen % 1024;
    //     client.write(fbBuf, remainder);
    //   }
    // }

    client.print(tail);
    Serial.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis())
    {
      Serial.print(".");
      delay(100);
      while (client.available())
      {
        char c = client.read();
        if (c == '\n')
        {
          if (getAll.length() == 0)
          {
            state = true;
          }
          getAll = "";
        }
        else if (c != '\r')
        {
          getAll += String(c);
        }
        if (state == true)
        {
          getBody += String(c);
        }
        startTimer = millis();
      }
      if (getBody.length() > 0)
      {
        break;
      }
    }
    Serial.println();
    client.stop();
    Serial.println(getBody);
  }
  else
  {
    getBody = "Connection to " + SERVER + " failed.";
    Serial.println(getBody);
  }
  return getBody;
}

String headerNotifSaldo(char *sn)
{
  // POST /notif/lowsaldo/007 HTTP/1.1
  // Accept: */*
  // User-Agent: Thunder Client (https://www.thunderclient.com)
  // Content-Type: multipart/form-data; boundary=---011000010111000001101001
  // Host: 203.194.112.238:3300

  String serialnum = String(sn);
  String data;
  data = "POST /notif/lowsaldo/" + serialnum + " HTTP/1.1\r\n";
  data += "Accept: */*\r\n";
  data += "User-Agent: PostmanRuntime/6.4.1\r\n";
  data += "Content-Type: multipart/form-data; boundary=---011000010111000001101001";
  data += "\r\n";
  data += "Host: ";
  data += SERVER + ":" + String(PORT);
  data += "\r\n";
  data += "\r\n";
  data += "\r\n";
  return (data);
}

String sendNotifSaldo(char *sn)
{
  String serverRes;
  String headerTxt = headerNotifSaldo(sn);

  if (!client.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  client.print(headerTxt);
  Serial.print(headerTxt);

  delay(20);
  long tOut = millis() + TIMEOUT;
  long startTimer = millis();
  boolean state = false;
  String getAll;
  String getBody;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    while (client.available())
    {
      char c = client.read();
      if (c == '\n')
      {
        if (getAll.length() == 0)
        {
          state = true;
        }
        getAll = "";
      }
      else if (c != '\r')
      {
        getAll += String(c);
      }
      if (state == true)
      {
        getBody += String(c);
      }
      startTimer = millis();
    }
    if (getBody.length() > 0)
    {
      break;
    }
  }
  client.stop();
  return getBody;
}

void loop()
{
  // Serial.print("Main Loop: Executing on core ");
  // Serial.println(xPortGetCoreID());
}
