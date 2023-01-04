#include <Arduino.h>
#include <SettingsManager.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <BluetoothSerial.h>
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include <PubSubClient.h>
#include <Wire.h>
#include <ElegantOTA.h>
#include "esp_camera.h"
#include "EEPROM.h"

#include <Adafruit_MCP23017.h>
#include <driver/i2s.h>
#include <math.h>
#include <arduinoFFT.h>
#include <ESP32Ping.h>

#include <TimeLib.h>
#include <TimeAlarms.h>
#include <timeout.h>

#include <FS.h>
#include <SPIFFS.h>

AlarmId pingTime;
AlarmId pingTimeSend;
Adafruit_MCP23017 mcp;
int counter = 0;
bool soundGagalTimeout = false;

#define EEPROM_SIZE 128

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

#define PIN_MCP_SCL 15
#define PIN_MCP_SDA 14
#define PIN_SOUND_SCK 13 // BCK // CLOCK
#define PIN_SOUND_WS 2   // WS
#define PIN_SOUND_SD 12  // DATA IN

#define SOL_0 0
#define SOL_1 1
#define SOL_2 2
#define SOL_3 3
#define SOL_3 3
#define SOL_4 4
#define SOL_5 5
#define SOL_6 6
#define SOL_7 7
#define SOL_8 8
#define SOL_9 9
#define SOL_BACK 10
#define SOL_ENTER 11

#define I2CADDR 0x20

SettingsManager settings;
BluetoothSerial SerialBT;
WiFiClient wificlient;
PubSubClient mqtt(wificlient);
// WebServer OTASERVER(80);
IPAddress mqttServer(203, 194, 112, 238);
Timeout timergagal;
Timeout timer;
#define ITRON
// #define HEXING
#ifdef ITRON
int delayStatus = 700;
int delayNominal = 1700;
int delaySaldo = 4000;
#endif
#ifdef HEXING
int delayStatus = 1100;
int delayNominal = 5200;
int delaySaldo = 11000;
#endif

enum class Status : uint8_t
{
  NONE,
  IDLE,
  GET_KWH,
  PROCCESS_TOKEN,
  SEND_IMAGE_SALDO,
  SEND_IMAGE_NOMINAL,
  NOTIF_LOW_SALDO,
};

Status status = Status::NONE;

const char *configFile = "/config.json";
char productName[16] = "TOKEN-ONLINE_";
char wifissidbuff[20];
char wifipassbuff[20];
const char *wifissid = "MGI-MNC";
const char *wifipass = "#neurixmnc#";

// const char *wifissid = "NEURIXII";
// const char *wifipass = "#neurix123#";

char idDevice[9] = "007";
char bluetooth_name[sizeof(productName) + sizeof(idDevice)];
const char *mqttChannel = "TokenOnline";
const char *MqttUser = "das";
const char *MqttPass = "mgi2022";

char topicREQToken[50];
char topicRSPToken[50];
char topicRSPSound[50];
char topicREQView[50];
char topicRSPView[50];
char topicRSPSignal[50];
char topicRSPStatus[50];

char PrevToken[21];
char NowToken[21];

long start_wifi_millis;
long wifi_timeout = 10000;
long tOut;
int cntDetected;
int cntLoss;

unsigned int SoundPeak;

#define SAMPLES 1024
const i2s_port_t I2S_PORT = I2S_NUM_1;
const int BLOCK_SIZE = SAMPLES;
#define OCTAVES 9
#define SOUND_TIMEOUT 3000

// our FFT data
static float real[SAMPLES];
static float imag[SAMPLES];
static arduinoFFT fft(real, imag, SAMPLES, SAMPLES);
static float energy[OCTAVES];

// A-weighting curve from 31.5 Hz ... 8000 Hz
static const float aweighting[] = {-39.4, -26.2, -16.1, -8.6, -3.2, 0.0, 1.2, 1.0, -1.1};
static unsigned int bell = 0;
static unsigned int hexingShort = 0;
static unsigned int hexingLong = 0;
static unsigned int itronShortFast = 0;
static unsigned int itronBerhasilKantor = 0;
static unsigned int itronGagalKantor = 0;
static unsigned long ts = millis();
static unsigned long last = micros();
static unsigned int sum = 0;
static unsigned int mn = 9999;
static unsigned int mx = 0;
static unsigned int cnts = 0;
static unsigned long lastTrigger[2] = {0, 0};

boolean detectShort = false;
boolean detectLong = false;
int detectLongCnt;
int detectShortCnt;

int cnt = 0;

enum ENUM_KONEKSI
{
  NONE,
  BT_INIT,
  WIFI_INIT,
  WIFI_OK,
  WIFI_FAILED
};

enum ENUM_KONEKSI KoneksiStage = NONE;

char received_topic[128];
char received_payload[128];
unsigned int received_length;
bool received_msg = false;

const String serverName = "203.194.112.238";      // REPLACE WITH YOUR Raspberry Pi IP ADDRESS OR DOMAIN NAME
const String topupEndpoint = "/topup/response/?"; // Needs to match upload server endpoint
const String topupBodyNominal = "\"imgNominal\"";
const String topupBodySaldo = "\"imgSaldo\"";
const String topupFileNominal = "\"imgNominal.jpg\"";
const String topupFileSaldo = "\"imgSaldo.jpg\"";

const String saldoEndpoint = "/saldo/response/?";
const String saldoBody = "\"imgBalance\"";
const String saldoFileName = "\"imgBalance.jpg\"";

const String notifEndpoint = "/notif/lowsaldo/?";
const String keyName = "\"file\""; // Needs to match upload server keyName
const int serverPort = 3300;

// send photo char-status->berhasil ,char-pelanggan->01 ,char-sn->222 ,char-token->12313221414355355552
// return json body

// dummy
const String SERVER = "203.194.112.238";
// const String SERVER = "192.168.0.190";
int PORT = 3300;
#define BOUNDARY "--------------------------133747188241686651551404"
#define TIMEOUT 20000
// dummy

char *sPtr[20];
char *strData = NULL; // this is allocated in separate and needs to be free( ) eventually

void MqttCustomPublish(char *path, String msg);
void MqttPublishStatus(String msg);
bool MechanicTyping(int pin)
{
  mcp.digitalWrite(pin, HIGH);
  delay(150);
  mcp.digitalWrite(pin, LOW);
  return true;
}

void MechanicInit()
{
  Wire.begin(PIN_MCP_SDA, PIN_MCP_SCL);
  mcp.begin(&Wire);
  for (int i = 0; i < 16; i++)
  {
    mcp.pinMode(i, OUTPUT); // MCP23017 pins are set for inputs
    // delay(10);
  }
  for (int i = 0; i < 16; i++)
  {
    MechanicTyping(i);
    // delay(10);
  }
}

void MechanicEnter()
{
  MechanicTyping(11);
}

bool SettingsInit()
{
  int response;
  settings.readSettings(configFile);
  strcpy(idDevice, settings.getChar("device.id"));
  Serial.println(idDevice);
  Serial.println(settings.getChar("bluetooth.name"));

  if (strlen(settings.getChar("bluetooth.name")) == 0)
  {
    Serial.println(" ZERO BT NAME");
    response = settings.setChar("bluetooth.name", strcat(productName, idDevice));
    settings.writeSettings(configFile);
    ESP.restart();
  }
  else
    strcpy(bluetooth_name, settings.getChar("bluetooth.name"));

  Serial.println(idDevice);
  Serial.println(bluetooth_name);
  strcpy(wifissidbuff, settings.getChar("wifi.ssid"));
  strcpy(wifipassbuff, settings.getChar("wifi.pass"));
  Serial.println(wifissidbuff);
  Serial.println(wifipassbuff);

  return true;
}

bool SettingsWifiSave(char *ssid, char *pass)
{
  settings.setChar("wifi.ssid", ssid);
  settings.setChar("wifi.pass", pass);

  Serial.println("---------- Save settings ----------");
  settings.writeSettings("/config.json");
  Serial.println("---------- Read settings from file ----------");
  settings.readSettings("/config.json");
  Serial.println("---------- New settings content ----------");

  return false;
}

int WiFiFormParsing(String str, // pass as a reference to avoid double memory usage
                    char **p, int size, char **pdata)
{
  int n = 0;
  free(*pdata); // clean up last data as we are reusing sPtr[ ]
  // BE CAREFUL not to free it twice
  // calling free on NULL is OK
  *pdata = strdup(str.c_str()); // allocate new memory for the result.
  if (*pdata == NULL)
  {
    Serial.println("OUT OF MEMORY");
    return 0;
  }
  *p++ = strtok(*pdata, ";");
  for (n = 1; NULL != (*p++ = strtok(NULL, ";")); n++)
  {
    if (size == n)
    {
      break;
    }
  }
  return n;
}

int CheckPingGateway()
{
  int cntLoss;
  int pingReturn;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Ping.ping(WiFi.localIP()))
      cntLoss = 0;
    else
      cntLoss++;
  }
  if (cntLoss > 7)
    pingReturn = 0;
  else if (cntLoss = 0)
    pingReturn = 100;
  else
    pingReturn = 50;
  return cntLoss;
}

void CheckPingServer()
{
  int pingReturn;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Ping.ping(mqttServer), 2)
      cntLoss = 0;
    else
      cntLoss++;
  }
  Serial.printf("CheckPingServer() : cntLoss=%d\n", cntLoss);
}

void CheckPingSend()
{
  if (cntLoss == 3)
  {
    MqttCustomPublish(topicRSPSignal, "1");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 1"));
    cntLoss = 0;
  }
  else if (cntLoss == 2)
  {
    MqttCustomPublish(topicRSPSignal, "2");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 2"));
    cntLoss = 0;
  }
  else if (cntLoss <= 1)
  {
    MqttCustomPublish(topicRSPSignal, "3");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 3"));
    cntLoss = 0;
  }
}

void CheckPingBackground(void *parameter)
{
  pingTime = Alarm.timerRepeat(10, CheckPingServer);
  Alarm.alarmOnce(10, CheckPingSend);
  pingTimeSend = Alarm.timerRepeat(30, CheckPingSend);
  for (;;)
  {
    // Serial.print("Main Ping: Executing on core ");
    // Serial.println(xPortGetCoreID());
    Alarm.delay(1000);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void dummySignalSend()
{
  MqttCustomPublish(topicRSPSignal, "3");
}

void BluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  char serialBTBuffer[50];

  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("BT Client Connected");
    KoneksiStage = BT_INIT;
  }

  if (event == ESP_SPP_DATA_IND_EVT and KoneksiStage == BT_INIT)
  {
    int N = WiFiFormParsing(SerialBT.readString(), sPtr, 20, &strData);
    if (N == 2)
    {
      strcpy(wifissidbuff, sPtr[0]);
      strcpy(wifipassbuff, sPtr[1]);
      // }
      Serial.println(wifissidbuff);
      Serial.println(wifipassbuff);
      SettingsWifiSave(wifissidbuff, wifipassbuff);
      ESP.restart();
    }
  }
}

void KoneksiInit()
{
  SerialBT.begin(bluetooth_name);
  SerialBT.register_callback(BluetoothCallback);

  // xTaskCreatePinnedToCore(
  //     CheckPingBackground,
  //     "pingServer",     // Task name
  //     5000,             // Stack size (bytes)
  //     NULL,             // Parameter
  //     tskIDLE_PRIORITY, // Task priority
  //     NULL,             // Task handle
  //     0);
}

void WifiInit()
{
  if (strcmp(wifipassbuff, "NULL") == 0)
    WiFi.begin(wifissidbuff, NULL);
  else
    WiFi.begin(wifissidbuff, wifipassbuff);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (millis() - start_wifi_millis > wifi_timeout)
    {
      WiFi.disconnect(true, true);
    }
  }
  // WiFi.setAutoReconnect(true);
  // WiFi.persistent(true);
  Serial.println("WifiInit()...Successfull");
  Serial.print("WiFi connected to");
  Serial.print(wifissidbuff);
  Serial.print(" IP: ");
  Serial.println(WiFi.localIP());

  // OTASERVER.on("/", []()
  //              { OTASERVER.send(200, "text/plain", "Hi! I am ESP8266."); });

  // ElegantOTA.begin(&OTASERVER); // Start ElegantOTA
  // OTASERVER.begin();
}

void MqttCustomPublish(char *path, String msg);

String sendPhotoToken(char *status, char *sn, char *token)
{
  String getAll;
  String getBody;
  WiFiClient postClient;
  camera_fb_t *fb = NULL;

  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb); // dispose the buffered image
  fb = NULL;                // reset to capture errors
  fb = esp_camera_fb_get(); // get fresh image
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Connecting to server: " + serverName);

  if (postClient.connect(serverName.c_str(), serverPort))
  {
    Serial.println("Connection successful!");
    String headNominal = "--MGI\r\nContent-Disposition: form-data; name=" + topupBodyNominal +
                         "; filename=" + topupFileNominal + "\r\nContent-Type: image/jpeg\r\n\r\n";
    String headSaldo = "--MGI\r\nContent-Disposition: form-data; name=" + topupBodySaldo +
                       "; filename=" + topupFileSaldo + "\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--MGI--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = headNominal.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    postClient.println("POST " + topupEndpoint + "status=" + status +
                       "&sn_device=" + sn + "&token=" + token + " HTTP/1.1");
    Serial.println("POST " + topupEndpoint + "status=" + status +
                   "&sn_device=" + sn + "&token=" + token + " HTTP/1.1");

    postClient.println("Host: " + serverName);
    Serial.println("Host: " + serverName);
    postClient.println("Content-Length: " + String(totalLen));
    Serial.println("Content-Length: " + String(totalLen));
    postClient.println("Content-Type: multipart/form-data; boundary=MGI");
    Serial.println("Content-Type: multipart/form-data; boundary=MGI");
    postClient.println();
    Serial.println();

    postClient.print(headNominal);
    Serial.print(headNominal);
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
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

    postClient.println();
    Serial.println();
    postClient.print(headSaldo);
    Serial.print(headSaldo);

    // uint8_t *fbBuf = fb->buf;
    // size_t fbLen = fb->len;
    // for (size_t n = 0; n < fbLen; n = n + 1024)
    // {
    //   if (n + 1024 < fbLen)
    //   {
    //     postClient.write(fbBuf, 1024);
    //     fbBuf += 1024;
    //   }
    //   else if (fbLen % 1024 > 0)
    //   {
    //     size_t remainder = fbLen % 1024;
    //     postClient.write(fbBuf, remainder);
    //   }
    // }

    postClient.print(tail);
    Serial.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis())
    {
      Serial.print(".");
      delay(100);
      while (postClient.available())
      {
        char c = postClient.read();
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
    // postClient.stop();
    Serial.println(getBody);
  }
  else
  {
    getBody = "Connection to " + serverName + " failed.";
    Serial.println(getBody);
  }
  return getBody;
}

String sendPhotoViewKWH(char *sn)
{
  String getAll;
  String getBody;
  WiFiClient postClient;
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb); // dispose the buffered image
  fb = NULL;                // reset to capture errors
  fb = esp_camera_fb_get(); // get fresh image
  // camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Connecting to server: " + serverName);

  if (postClient.connect(serverName.c_str(), serverPort))
  {
    Serial.println("Connection successful!");
    String head = "--MGI\r\nContent-Disposition: form-data; name=" + saldoBody + "; filename=" + saldoFileName + "\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--MGI--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    postClient.println("POST " + saldoEndpoint + "&sn_device=" + sn + " HTTP/1.1");
    Serial.println("POST " + saldoEndpoint + "&sn_device=" + sn + " HTTP/1.1");

    postClient.println("Host: " + serverName);
    postClient.println("Content-Length: " + String(totalLen));
    postClient.println("Content-Type: multipart/form-data; boundary=MGI");
    postClient.println();
    postClient.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
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
    postClient.print(tail);
    esp_camera_fb_return(fb);
    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis())
    {
      Serial.print(".");
      delay(100);
      while (postClient.available())
      {
        char c = postClient.read();
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
    // postClient.stop();
    Serial.println(getBody);
  }
  else
  {
    getBody = "Connection to " + serverName + " failed.";
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
  WiFiClient postClient;
  String headerTxt = headerNotifSaldo(sn);
  MqttPublishStatus("SEND NOTIF SALDO");
  if (!postClient.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  MqttPublishStatus("SENDING NOTIF SALDO");
  Serial.println("SENDING NOTIF SALDO");

  postClient.print(headerTxt);

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
    while (postClient.available())
    {
      char c = postClient.read();
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
  // WifiEspClient.stop();
  return getBody;
}

char *extract_between(const char *str, const char *p1, const char *p2)
{
  const char *i1 = strstr(str, p1);
  if (i1 != NULL)
  {
    const size_t pl1 = strlen(p1);
    const char *i2 = strstr(i1 + pl1, p2);
    if (p2 != NULL)
    {
      /* Found both markers, extract text. */
      const size_t mlen = i2 - (i1 + pl1);
      char *ret = (char *)malloc(mlen + 1);
      if (ret != NULL)
      {
        memcpy(ret, i1 + pl1, mlen);
        ret[mlen] = '\0';
        return ret;
      }
    }
  }
}

void printDeviceAddress()
{
  const uint8_t *point = esp_bt_dev_get_address();
  for (int i = 0; i < 6; i++)
  {
    char str[3];
    sprintf(str, "%02X", (int)point[i]);
    Serial.print(str);

    if (i < 5)
    {
      Serial.print(":");
    }
  }
}

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
    return;
  }
  else
    Serial.printf("CameraInit()...Succesfull\n", err);

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV5640_PID)
  {
    s->set_vflip(s, 1);
    s->set_brightness(s, -2);
  }
}

bool CheckTimeout(unsigned long *timeNow, unsigned long *timeStart)
{
  if (*timeNow - *timeStart >= SOUND_TIMEOUT)
    return true;
  else
    return false;

} // CheckTimeout

void SoundInit()
{
  Serial.println("Configuring I2S...");
  esp_err_t err;
  const i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX), // Receive, not transfer
      .sample_rate = 22627,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // for old esp-idf versions use RIGHT
      // .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S_MSB), // | I2S_COMM_FORMAT_I2S),I2S_COMM_FORMAT_STAND_I2S
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Interrupt level 1
      .dma_buf_count = 8,                       // number of buffers
      .dma_buf_len = BLOCK_SIZE,                // samples per buffer
      .use_apll = true};
  // The pin config as per the setup
  const i2s_pin_config_t pin_config = {
      .bck_io_num = PIN_SOUND_SCK, // BCKL
      .ws_io_num = PIN_SOUND_WS,   // LRCL
      .data_out_num = -1,          // not used (only for speakers)
      .data_in_num = PIN_SOUND_SD  // DOUTESP32-INMP441 wiring
  };
  // Configuring the I2S driver and pins.
  // This function must be called before any I2S driver read/write operations.
  err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK)
  {
    Serial.printf("Failed installing driver: %d\n", err);
    while (true)
      ;
  }
  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK)
  {
    Serial.printf("Failed setting pin: %d\n", err);
    while (true)
      ;
  }
  Serial.println("I2S driver installed.");
}

static void integerToFloat(int32_t *integer, float *vReal, float *vImag, uint16_t samples)
{
  for (uint16_t i = 0; i < samples; i++)
  {
    vReal[i] = (integer[i] >> 16) / 10.0;
    vImag[i] = 0.0;
  }
}
// calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
static void calculateEnergy(float *vReal, float *vImag, uint16_t samples)
{
  for (uint16_t i = 0; i < samples; i++)
  {
    vReal[i] = sq(vReal[i]) + sq(vImag[i]);
    vImag[i] = 0.0;
  }
}
// sums up energy in bins per octave
static void sumEnergy(const float *bins, float *energies, int bin_size, int num_octaves)
{
  // skip the first bin
  int bin = bin_size;
  for (int octave = 0; octave < num_octaves; octave++)
  {
    float sum = 0.0;
    for (int i = 0; i < bin_size; i++)
    {
      sum += real[bin++];
    }
    energies[octave] = sum;
    bin_size *= 2;
  }
}
static float decibel(float v)
{
  return 10.0 * log(v) / log(10);
}
// converts energy to logaritmic, returns A-weighted sum
static float calculateLoudness(float *energies, const float *weights, int num_octaves, float scale)
{
  float sum = 0.0;
  for (int i = 0; i < num_octaves; i++)
  {
    float energy = scale * energies[i];
    sum += energy * pow(10, weights[i] / 10.0);
    energies[i] = decibel(energy);
  }
  return decibel(sum);
}

unsigned int countSetBits(unsigned int n)
{
  unsigned int count = 0;
  while (n)
  {
    count += n & 1;
    n >>= 1;
  }
  return count;
}
// detecting 2 frequencies. Set wide to true to match the previous and next bin as well
bool detectFrequency(unsigned int *mem, unsigned int minMatch, double peak, unsigned int bin1, unsigned int bin2, bool wide)
{
  *mem = *mem << 1;

  if (peak == bin1 || peak == bin2 || (wide && (peak == bin1 + 1 || peak == bin1 - 1 || peak == bin2 + 1 || peak == bin2 - 1)))
  {
    *mem |= 1;
  }
  if (countSetBits(*mem) >= minMatch)
  {
    return true;
  }
  return false;
}

void calculateMetrics(int val)
{
  cnt++;
  sum += val;
  if (val > mx)
  {
    mx = val;
  }
  if (val < mn)
  {
    mn = val;
  }
}

void SoundLooping()
{
  static int32_t samples[BLOCK_SIZE];
  // Read multiple samples at once and calculate the sound pressure
  size_t num_bytes_read;
  esp_err_t err = i2s_read(I2S_PORT,
                           (char *)samples,
                           BLOCK_SIZE, // the doc says bytes, but its elements.
                           &num_bytes_read,
                           portMAX_DELAY); // no timeout
  int samples_read = num_bytes_read / 8;
  // integer to float
  integerToFloat(samples, real, imag, SAMPLES);
  // apply flat top window, optimal for energy calculations
  fft.Windowing(FFT_WIN_TYP_FLT_TOP, FFT_FORWARD);
  fft.Compute(FFT_FORWARD);
  // calculate energy in each bin
  calculateEnergy(real, imag, SAMPLES);
  // sum up energy in bin for each octave
  sumEnergy(real, energy, 1, OCTAVES);
  // calculate loudness per octave + A weighted loudness
  float loudness = calculateLoudness(energy, aweighting, OCTAVES, 1.0);
  SoundPeak = (int)floor(fft.MajorPeak());

  // Serial.println(peak);
  // Serial.printf("%d %d %d \n", peak, detectLong, detectShort);

  // detectLong = detectFrequency(&bell, 22, peak, 117, 243, true);
  // detectShort = detectFrequency(&bell, 2, peak, 117, 243, true);

  // if (detectLong)
  //   Serial.println("Detect Long");
  // else if (detectShort)
  //   Serial.println("Detect Short");
}

bool SoundDetectLong()
{
  SoundLooping();
  bool detectedLong = detectFrequency(&bell, 22, SoundPeak, 117, 243, true);
  bool detectedShort = detectFrequency(&bell, 2, SoundPeak, 117, 243, true);

  // Serial.printf("%d %d %d %d %d\n", SoundPeak, detectedLong, detectedShort, detectLongCnt, detectShortCnt);
  if (!detectedShort and (detectShortCnt > 35) or detectedLong)
  {
    Serial.println("DETECTED LONG BEEP BEEP");
    cntDetected = 0;
    detectShortCnt = 0;
    detectLongCnt = 0;
    return true;
  }
  else
  {
    if (detectedLong)
    {
      cntDetected++;
      detectLongCnt++;
    }
    // Serial.println("DETEK LONG BEEP");
    else if (detectedShort) // and !detectLong)
    {
      cntDetected++;
      detectShortCnt++;
    }
    else
    {
      cntDetected = 0;
      detectShortCnt = 0;
      detectLongCnt = 0;
    }

    return false;
  }
}

bool SoundDetectShort()
{
  SoundLooping();
  bool detectedLong = detectFrequency(&bell, 6, SoundPeak, 117, 243, true);
  bool detectedShort = detectFrequency(&itronShortFast, 6, SoundPeak, 425, 35, true);

  // Serial.printf("%d %d %d %d %d\n", SoundPeak, detectedLong, detectedShort, detectLongCnt, detectShortCnt);
  if (!detectedShort and (detectShortCnt > 1 and detectShortCnt < 35))
  {
    Serial.println("DETECTED SHORT BEEP");
    cntDetected = 0;
    detectShortCnt = 0;
    detectLongCnt = 0;
    return true;
  }
  else
  {
    if (detectedLong)
    {
      cntDetected++;
      detectLongCnt++;
    }
    // Serial.println("DETEK LONG BEEP");
    else if (detectedShort) // and !detectLong)
    {
      cntDetected++;
      detectShortCnt++;
    }
    else
    {
      cntDetected = 0;
      detectShortCnt = 0;
      detectLongCnt = 0;
    }
    MqttCustomPublish("DeteckShort", String(detectedShort));
    MqttCustomPublish("soundShortcnt", String(SoundPeak));

    return false;
  }
}

bool SoundDetectShortItron()
{
  SoundLooping();
  bool detectedLong = detectFrequency(&hexingLong, 22, SoundPeak, 117, 243, true);
  // bool detectedShort = detectFrequency(&hexingShort, 2, SoundPeak, 117, 243, true);
  bool detectedShortItronFast = detectFrequency(&itronShortFast, 6, SoundPeak, 182, 199, true);

  // Serial.printf("%d %d %d %d %d\n", SoundPeak, detectedLong, detectedShortItronFast, detectLongCnt, detectShortCnt);
  if (detectedShortItronFast and (detectShortCnt > 100 /*and detectShortCnt < 35)*/))
  {
    MqttCustomPublish(topicRSPSound, "DETECTED SHORT BEEP");
    Serial.println("DETECTED SHORT BEEP");
    cntDetected = 0;
    detectShortCnt = 0;
    detectLongCnt = 0;
    return true;
  }
  else
  {
    // if (detectedLong)
    // {
    //   cntDetected++;
    //   detectLongCnt++;
    // }
    // Serial.println("DETEK LONG BEEP");
    // MqttCustomPublish("sound", "ELSE DETECTED SHORT BEEP");
    if (detectedShortItronFast) // and !detectLong)
    {
      cntDetected++;
      detectShortCnt++;
    }
    else
    {
      cntDetected = 0;
      detectShortCnt = 0;
      detectLongCnt = 0;
    }
    // MqttCustomPublish("DeteckShort", String(detectedShortItronFast));
    // MqttCustomPublish("soundShortcnt", String(SoundPeak));
    return false;
  }
}

bool SoundDetectShortItronKantorGagal()
{
  bool returning = false;
  // timergagal.start();
  // while (state)
  // {
  int timoutTimer = 3000;
  long startTimer = millis();
  boolean state = false;

  SoundLooping();
  bool detectedShortGagal = detectFrequency(&itronGagalKantor, 3, SoundPeak, 169, 192, true);
  // bool detectedShortItronFast = detectFrequency(&itronShortFast, 6, SoundPeak, 168, 199, true);

  return detectedShortGagal;
  // }
}

void DetectGagalItronKantor()
{
  if (SoundDetectShortItron())
  {
    mqtt.publish("sound", "DETECT KANTOR");
    Serial.println("DETECK LOW TOKEN");
    Serial.println("SEND NOTIFICATION");
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  strcpy(received_topic, topic);
  memcpy(received_payload, payload, length);
  received_msg = true;
  received_length = length;
  Serial.println("void callback()");
  Serial.printf("received_topic %s received_length = %d \n", received_topic, received_length);
  Serial.printf("received_payload %s \n", received_payload);
  Serial.println("void callback()");
}

bool MqttConnect()
{
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
  char clientId[9];
  uint8_t retry = 3;
  while (!mqtt.connected())
  {
    Serial.println(String("Attempting MQTT broker:") + mqttServer);

    if (mqtt.connect(mqttChannel, MqttUser, MqttPass))
    {
      Serial.println("Established:" + String(clientId));
      mqtt.subscribe(topicREQToken);
      mqtt.subscribe(topicREQView);
      return true;
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      if (!--retry)
        break;
      // delay(3000);
    }
  }
  return false;
}

void MqttCustomPublish(char *path, String msg)
{
  mqtt.publish(path, msg.c_str());
}

void MqttPublishStatus(String msg)
{
  MqttCustomPublish(topicRSPStatus, msg);
}

void MqttTopicInit()
{
  // sprintf(topicREQToken, "%s/%s/%s/%s", mqttChannel, "request", idDevice, "token");
  sprintf(topicREQToken, "%s/%s/%s", mqttChannel, "request", idDevice);
  sprintf(topicREQView, "%s/%s/%s/%s", mqttChannel, "request", "view", idDevice);
  sprintf(topicRSPToken, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "token");
  sprintf(topicRSPView, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "view");
  sprintf(topicRSPSignal, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "signal");
  sprintf(topicRSPStatus, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "debug");
  sprintf(topicRSPSound, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "sound");

  Serial.printf("%s\n", topicREQToken);
  Serial.printf("%s\n", topicREQView);
  Serial.printf("%s\n", topicRSPSignal);
  Serial.printf("%s\n", topicRSPStatus);
  Serial.println(topicRSPToken);
  Serial.println(topicRSPView);
}

/* *********************************** MQTT ************************* */
void MqttReconnect()
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect("monTok"))
    {
      Serial.println("connected");
      MqttPublishStatus("CONNECTED MQTT");
      mqtt.subscribe(topicREQToken);
      mqtt.subscribe(topicREQView);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool DetectionLowToken()
{
  if (SoundDetectShortItron())
  {
    timer.start();
    counter++;
    Serial.printf("COUNTER SHORT DETECT : %d\n", counter);
    MqttCustomPublish(topicRSPSound, "COUNTER SOUND " + String(counter));
  }
  else if (timer.time_over())
  {
    // Serial.print("TIMEOUT...");
    // Serial.println("RESET");
    counter = 0;
  }

  if (/* timer.time_over()  and */ counter > 13)
  {
    MqttCustomPublish(topicRSPSound, "SEND NOTIFICATION SOUND");
    sendNotifSaldo(idDevice);
    Serial.println("DETECK LOW TOKEN");
    Serial.println("SEND NOTIFICATION");
    counter = 0;
  }
  return true;
}

void GetKwhProcess()
{
  if (received_msg and strcmp(received_topic, topicREQView) == 0 /*and strcmp(received_payload, "view") == 0*/)
  {
    status = Status::GET_KWH;
    MqttPublishStatus("REQUESTING BALANCE");
    Serial.println("GET_KWH");
    Serial.println(sendPhotoViewKWH(idDevice));
    // MqttCustomPublish(topicRSPView, "gwtkwh-success");
    received_msg = false;
    status = Status::IDLE;
  }
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

String headerStatusToken(char *sn, size_t length)
{
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/status/?sn_device=" + String(sn) + " HTTP/1.1\r\n";
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

// String sendImageNominal(char *id, uint8_t *data_pic, size_t size_pic)
// {
//   String serverRes;
//   String bodyNominal = body_Nominal();
//   WiFiClient postClient;
//   String bodyEnds = String("--") + BOUNDARY + String("--\r\n");
//   size_t allLen = bodyNominal.length() + size_pic + bodyEnds.length();
//   String headerTxt = headerNominal(id, allLen);

//   if (!postClient.connect(SERVER.c_str(), PORT))
//   {
//     return ("connection failed");
//   }

//   postClient.print(headerTxt + bodyNominal);
//   Serial.print(headerTxt + bodyNominal);
//   postClient.write(data_pic, size_pic);

//   postClient.print("\r\n" + bodyEnds);
//   Serial.print("\r\n" + bodyEnds);

//   delay(20);
//   long tOut = millis() + TIMEOUT;
//   while (tOut > millis())
//   {
//     Serial.print(".");
//     delay(100);
//     if (postClient.available())
//     {
//       serverRes = postClient.readStringUntil('\r');
//       return (serverRes);
//     }
//     else
//     {
//       serverRes = "NOT RESPONSE";
//       return (serverRes);
//     }
//   }
//   // postClient.stop();
//   return serverRes;
// }

String sendImageNominal(uint8_t *data_pic, size_t size_pic)
{
  String getAll;
  String getBody;
  WiFiClient postClient;

  Serial.println("Connecting to server: " + SERVER);

  if (postClient.connect(SERVER.c_str(), serverPort))
  {

    String serverRes;
    String bodyNominal = body_Nominal();
    WiFiClient postClient;
    String bodyEnds = String("--") + BOUNDARY + String("--\r\n");
    size_t allLen = bodyNominal.length() + size_pic + bodyEnds.length();
    String headerTxt = headerNominal(idDevice, allLen);

    if (!postClient.connect(SERVER.c_str(), PORT))
    {
      return ("connection failed");
    }

    postClient.print(headerTxt + bodyNominal);
    Serial.print(headerTxt + bodyNominal);

    uint8_t *fbBuf = data_pic;
    size_t fbLen = size_pic;
    // CaptureToSpiffs("/nominal.jpg", fb->buf, fb->len);
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
    Serial.print("\r\n" + bodyEnds);

    long tOut = millis() + TIMEOUT;
    while (tOut > millis())
    {
      Serial.print(".");
      delay(100);
      if (postClient.available())
      {
        serverRes = postClient.readStringUntil('\r');
        return (serverRes);
      }
      else
      {
        serverRes = "NOT RESPONSE";
        return (serverRes);
      }
    }
    // postClient.stop();
    return serverRes;
  }
  else
    return String("NOT CONNECT");
}

String sendImageSaldo(char *token, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  WiFiClient postClient;
  String bodySaldo = body_Saldo();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySaldo.length() + size_pic + bodyEnd.length();
  String headerTxt = headerSaldo(token, allLen);

  if (!postClient.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  postClient.print(headerTxt + bodySaldo);
  Serial.print(headerTxt + bodySaldo);
  postClient.write(data_pic, size_pic);

  postClient.print("\r\n" + bodyEnd);
  Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (postClient.available())
    {
      serverRes = postClient.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  // postClient.stop();
  return serverRes;
}

String sendImageStatusToken(char *sn, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  WiFiClient postClient;
  String bodySatus = body_StatusToken();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySatus.length() + size_pic + bodyEnd.length();
  String headerTxt = headerStatusToken(sn, allLen);

  if (!postClient.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  // MqttPublishStatus("SEND IMAGE STATUS TOKEN");
  postClient.print(headerTxt + bodySatus);
  Serial.print(headerTxt + bodySatus);
  postClient.write(data_pic, size_pic);

  postClient.print("\r\n" + bodyEnd);
  Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (postClient.available())
    {
      serverRes = postClient.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  // postClient.stop();
  return serverRes;
}

void CaptureToSpiffs(String path, uint8_t *data_pic, size_t size_pic)
{
  if (!SPIFFS.begin())
  {
    Serial.println("SD Card Mount Failed");
    return;
  }

  Serial.printf("Picture file name: %s\n", path.c_str());

  File file = SPIFFS.open(path.c_str(), FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file in writing mode");
  }
  else
  {
    file.write(data_pic, size_pic); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  file.close();
}

void TokenProcess(/*char *token*/)
{
  if (received_msg and (strcmp(received_topic, topicREQToken) == 0) and received_length == 20)
  {
    status = Status::PROCCESS_TOKEN;
    MqttPublishStatus("EXECUTING TOKEN");
    for (int i = 0; i < received_length; i++)
    {

      char chars = received_payload[i];
      Serial.printf("%d ", i);
      // MqttPublishStatus("SELENOID " + String(chars) + " ON");
      switch (chars)
      {
      case '0':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");

        // MechanicTyping(10);
        delay(2000);
        break;

      case '1':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(0);
        delay(2000);
        break;

      case '2':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(1);
        delay(2000);
        break;
      case '3':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(2);
        delay(2000);
        break;
      case '4':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(3);
        delay(2000);
        break;
      case '5':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(4);
        delay(2000);
        break;
      case '6':

        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(5);
        delay(2000);
        break;
      case '7':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(6);
        delay(2000);
        break;
      case '8':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(7);
        delay(2000);
        break;
      case '9':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MechanicTyping(8);
        delay(2000);
        break;
      default:
        break;
      }
    }
    // delay();
    delay(delayStatus);
    Serial.println("SOLENOID ENTER");
    // MechanicEnter();
    // delay(delayStatus);
    String res;

    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb); // dispose the buffered image
    fb = NULL;                // reset to capture errors
    fb = esp_camera_fb_get(); // get fresh image
    if (!fb)
    {
      Serial.println("Camera capture failed");
      delay(1000);
      ESP.restart();
    }
    else
    {
      res = sendImageStatusToken(idDevice, fb->buf, fb->len);
      Serial.println(res);
      esp_camera_fb_return(fb);
    }

    delay(delayNominal);
    fb = NULL;
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb); // dispose the buffered image
    fb = NULL;                // reset to capture errors
    fb = esp_camera_fb_get(); // get fresh image
    if (!fb)
    {
      Serial.println("Camera capture failed");
      delay(1000);
      ESP.restart();
    }
    else
    {
      res = sendImageNominal(fb->buf, fb->len);
      Serial.println(res);
      esp_camera_fb_return(fb);
    }

    delay(delaySaldo);
    fb = NULL;
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb); // dispose the buffered image
    fb = NULL;                // reset to capture errors
    fb = esp_camera_fb_get(); // get fresh image
    if (!fb)
    {
      Serial.println("Camera capture failed");
      delay(1000);
      ESP.restart();
    }
    else
    {
      res = sendImageSaldo(received_payload, fb->buf, fb->len);
      Serial.println(res);
      esp_camera_fb_return(fb);
    }

    Serial.println("TOKEN PROCESS END");

    received_msg = false;
  }
}

bool send = false;
void setup()
{
  Serial.begin(115200);
  // MechanicInit();
  SettingsInit();
  KoneksiInit();
  WifiInit();
  CameraInit();

  if (!SPIFFS.begin(false /* false: Do not format if mount failed */))
  {
    Serial.println("Failed to mount LittleFS");
  }

  SoundInit();
  // MechanicInit();
  mqtt.setServer(mqttServer, 1883);
  mqtt.setCallback(callback);

  MqttTopicInit();
  // timergagal.prepare(3000);
  timer.prepare(5000);

  Alarm.timerOnce(10, dummySignalSend);
  pingTimeSend = Alarm.timerRepeat(30, dummySignalSend);

  pinMode(33, OUTPUT);
  digitalWrite(33, LOW);
  status = Status::IDLE;
}

void loop()
{
  if (!mqtt.connected() /*and status == Status::IDLE*/)
  {
    MqttReconnect();
  }
  // if (status == Status::IDLE)
  mqtt.loop();

  TokenProcess();
  GetKwhProcess();

  DetectionLowToken();
  // SoundDetectShort();

  // OTASERVER.handleClient();

  Alarm.delay(0);
}