#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Wire.h>
#include <PubSubClient.h>
#include "esp_camera.h"
#include "LittleFS_SysLogger.h"
#define _FSYS LittleFS

WiFiClient client;
PubSubClient mqtt(client);
char *topicTest = "Dummy/topup1";

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
const char *idDevice = "008";
const String SERVER = "203.194.112.238";
IPAddress mqttServer(203, 194, 112, 238);
int PORT = 3300;
#define BOUNDARY "--------------------------133747188241686651551404"

const int payloadSize = 100;
const int topicSize = 128;
char received_topic[topicSize];
char received_payload[payloadSize];
unsigned int received_length;
bool received_msg = false;

// camera_fb_t *cap_status = NULL;
// camera_fb_t *cap_nomi = NULL;
// camera_fb_t *cap_saldo = NULL;
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
  // config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_VGA; // 1600x1200
    config.jpeg_quality = 10;
    config.fb_count = 1;
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
    ESP.restart();
    return;
  }
  else
    Serial.printf("CameraInit()...Succesfull\n", err);

  sensor_t *s = esp_camera_sensor_get();
}

void WiFiBegin()
{
  WiFi.disconnect();
  if (strcmp(wifipassbuff, "NULL") == 0)
    WiFi.begin(wifissidbuff, NULL);
  else
    WiFi.begin(wifissidbuff, wifipassbuff);
  return;

  uint8_t result = WiFi.waitForConnectResult();
  if (result == WL_NO_SSID_AVAIL || result == WL_CONNECT_FAILED)
  {
    Serial.println(PSTR("[Wi-Fi] Status: Could not connect to Wi-Fi AP"));
    // wifi_connect_task.enableDelayed(1000);
  }
  else if (result == WL_IDLE_STATUS)
  {
    Serial.println(PSTR("[Wi-Fi] Status: Idle | No IP assigned by DHCP Server"));
    WiFi.disconnect();
  }
  else if (result == WL_CONNECTED)
  {
    Serial.printf(PSTR("[Wi-Fi] Status: Connected | IP: %s\n"), WiFi.localIP().toString().c_str());
  }
}

void WiFiInit()
{
  WiFiBegin();
  WiFi.mode(wifi_mode_t::WIFI_STA);
  WiFi.onEvent(_wifi_event);
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

String body_StatusToken()
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  data += F("Content-Disposition: form-data; name=\"imgStatus\"; filename=\"picture.jpg\"\r\n");
  data += F("Content-Type: image/jpeg\r\n");
  data += F("\r\n");

  return (data);
}

String headerNominal(char *id, size_t length)
{
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/nominal/?sn_device=" + String(id) + " HTTP/1.1\r\n";
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

String headerStatusToken(String status, char *sn, size_t length)
{
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/status/?sn_device=" + String(sn) + "&status=" + status + " HTTP/1.1\r\n";
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

String sendImageNominal(uint8_t *data_pic, size_t size_pic)
{
  String getAll;
  String getBody;
  WiFiClient postClient;

  Serial.println("Connecting to server: " + SERVER);

  if (postClient.connect(SERVER.c_str(), PORT))
  {
    String serverRes;
    String bodyNominal = body_Nominal();
    WiFiClient postClient;
    String bodyEnds = String("--") + BOUNDARY + String("--\r\n");
    size_t allLen = bodyNominal.length() + size_pic + bodyEnds.length();
    String headerTxt = headerNominal("008", allLen);

    if (!postClient.connect(SERVER.c_str(), PORT))
    {
      return ("connection failed");
    }

    postClient.print(headerTxt + bodyNominal);

    uint8_t *fbBuf = data_pic;
    size_t fbLen = size_pic;
    for (size_t n = 0; n < fbLen; n = n + 1024)
    {
      if (n + 1024 < fbLen)
      {
        postClient.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0)
      {
        size_t remainder = fbLen % 1024;
        postClient.write(fbBuf, remainder);
      }
    }

    postClient.print("\r\n" + bodyEnds);
    // postClient.stop();

    return String("UPLOADED");
  }
  else
  {
    if (postClient.connect(SERVER.c_str(), PORT))
    {
      String serverRes;
      String bodyNominal = body_Nominal();
      WiFiClient postClient;
      String bodyEnds = String("--") + BOUNDARY + String("--\r\n");
      size_t allLen = bodyNominal.length() + size_pic + bodyEnds.length();
      String headerTxt = headerNominal((char *)idDevice, allLen);

      if (!postClient.connect(SERVER.c_str(), PORT))
      {
        return ("connection failed");
      }

      postClient.print(headerTxt + bodyNominal);
      uint8_t *fbBuf = data_pic;
      size_t fbLen = size_pic;
      for (size_t n = 0; n < fbLen; n = n + 1024)
      {
        if (n + 1024 < fbLen)
        {
          postClient.write(fbBuf, 1024);
          fbBuf += 1024;
        }
        else if (fbLen % 1024 > 0)
        {
          size_t remainder = fbLen % 1024;
          postClient.write(fbBuf, remainder);
        }
      }

      postClient.print("\r\n" + bodyEnds);
      // Serial.print("\r\n" + bodyEnds);
      // postClient.stop();

      return String("UPLOADED");
    }
    else
      return String("NOT UPLOADED");
  }
}

String sendImageSaldo(char *token, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  WiFiClient postClient;
  String bodySaldo = body_Saldo();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySaldo.length() + size_pic + bodyEnd.length();
  String headerTxt = headerSaldo(token, allLen);

  if (postClient.connect(SERVER.c_str(), PORT))
  {
    postClient.print(headerTxt + bodySaldo);
    postClient.write(data_pic, size_pic);

    postClient.print("\r\n" + bodyEnd);
    // postClient.stop();

    return String("UPLOADED");
  }
  else
  {
    if (postClient.connect(SERVER.c_str(), PORT))
    {
      postClient.print(headerTxt + bodySaldo);
      postClient.write(data_pic, size_pic);
      postClient.print("\r\n" + bodyEnd);
      // postClient.stop();
      return String("UPLOADED");
    }
    else
      return String("NOT UPLOADED");
  }
}

String sendImageStatusToken(String status, char *sn, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  WiFiClient postClient;
  String bodySatus = body_StatusToken();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySatus.length() + size_pic + bodyEnd.length();
  String headerTxt = headerStatusToken(status, sn, allLen);

  if (postClient.connect(SERVER.c_str(), PORT))
  {
    postClient.print(headerTxt + bodySatus);
    postClient.write(data_pic, size_pic);
    postClient.print("\r\n" + bodyEnd);
    // postClient.stop();

    return String("UPLOADED");
  }
  else
  {
    if (postClient.connect(SERVER.c_str(), PORT))
    {
      postClient.print(headerTxt + bodySatus);
      postClient.write(data_pic, size_pic);
      postClient.print("\r\n" + bodyEnd);
      // postClient.stop();
      return String("UPLOADED");
    }
    else
      return String("NOT UPLOADED");
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  memset(received_payload, '\0', payloadSize); // clear payload char buffer
  memset(received_topic, '\0', topicSize);
  int i = 0; // extract payload
  for (i; i < length; i++)
  {
    received_payload[i] = ((char)payload[i]);
  }
  received_payload[i] = '\0';
  strcpy(received_topic, topic);
  received_msg = true;
  received_length = length;
  Serial.printf("received_topic %s received_length = %d \n", received_topic, received_length);
  Serial.printf("received_payload %s \n", received_payload);
}

void MqttReconnect()
{
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "wnedwndw" + String(idDevice);
    if (mqtt.connect(clientId.c_str()))
    {
      Serial.println("connected");
      mqtt.subscribe(topicTest);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
    }
  }
}

void ReceiveTopup()
{
  if (received_msg and strcmp(received_topic, topicTest) == 0)
  {

    camera_fb_t *fbstatus = NULL;
    fbstatus = esp_camera_fb_get();
    if (!fbstatus)
    {
      ESP.restart();
    }
    else
    {

      String resp;
      resp = sendImageStatusToken("invalid", (char *)idDevice, fbstatus->buf, fbstatus->len);
      Serial.println(resp);
    }
    esp_camera_fb_return(fbstatus);
    camera_fb_t *fbnominal = NULL;
    fbnominal = esp_camera_fb_get();
    if (!fbnominal)
    {
      ESP.restart();
    }
    else
    {
      String resp;
      resp = sendImageNominal(fbnominal->buf, fbnominal->len);
      Serial.println(resp);
    }
    esp_camera_fb_return(fbnominal);
    camera_fb_t *fbsaldo = NULL;
    fbsaldo = esp_camera_fb_get();
    if (!fbsaldo)
    {
      ESP.restart();
    }
    else
    {
      String resp;
      resp = sendImageSaldo("11", fbsaldo->buf, fbsaldo->len);
      Serial.println(resp);
    }
    esp_camera_fb_return(fbsaldo);
    received_msg = false;
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void setup()
{
  Serial.begin(115200);
  CameraInit();
  WiFiInit();
  mqtt.setServer(mqttServer, 1883);
  mqtt.setCallback(callback);
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!mqtt.connected())
      MqttReconnect();
    ReceiveTopup();
    mqtt.loop();
  }
}