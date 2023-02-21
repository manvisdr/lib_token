#include <Arduino.h>
// USING WIFI
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Wire.h>
#include <PubSubClient.h>
// USING WIFI

// BLUETOOTH
#include <BluetoothSerial.h>
// BLUETOOTH

// TIMING
#include <ESPPerfectTime.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <timeout.h>
// TIMING

// HARDWARE
#include "esp_camera.h"
#include <Adafruit_MCP23017.h>
// HARDWARE

// SOUND
#include <driver/i2s.h>
#include <math.h>
#include <arduinoFFT.h>
// SOUND

// FILESYSTEM
#include <FS.h>
// #define litFS
#define SpifFS
// #ifdef litFS
// #include <LittleFS.h>
// #include "LittleFS_SysLogger.h"
// #define fileSys LittleFS
// #endif
#ifdef SpifFS
#include <SPIFFS.h>
#include "SPIFFS_SysLogger.h"
#define fileSys SPIFFS
#endif

#include <SettingsManager.h>
#ifdef USE_LOG_SYSTEM
ESPSL sysLog;
#endif
// FILESYSTEM

WiFiClient client;
BluetoothSerial SerialBT;
PubSubClient mqtt(client);
Adafruit_MCP23017 mcp;
SettingsManager settings;
ESPSL sysLog;

#define USE_SOLENOID
#define USE_LOG
#define PIN_MCP_SCL 15
#define PIN_MCP_SDA 14
#define PIN_SOUND_SCK 13 // BCK // CLOCK
#define PIN_SOUND_WS 2   // WS
#define PIN_SOUND_SD 12  // DATA IN

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

char topicTest[] = "TokenOnline/request/008";
const char ntpServer[] = "0.id.pool.ntp.org";
const String SERVER = "203.194.112.238";
IPAddress mqttServer(203, 194, 112, 238);
int PORT = 3300;
#define BOUNDARY "--------------------------133747188241686651551404"

// MQTT ADMIN
char productName[16] = "TOKEN-ONLINE_";
char configFile[] = "/config.json";
char bluetooth_name[30];
const char *mqttChannel = "TokenOnline";
const char *MqttUser = "das";
const char *MqttPass = "mgi2022";
// MQTT ADMIN
// MQTT VAR
const int payloadSize = 100;
const int topicSize = 128;
char received_topic[topicSize];
char received_payload[payloadSize];
unsigned int received_length;
bool received_msg = false;
// MQTT VAR
// TOPIC MQTT VAR
char topicREQToken[50];
char topicREQAlarm[50];
char topicREQView[50];
char topicREQNotif[50];
char topicREQtimeStatus[50];
char topicREQtimeNomi[50];
char topicREQstatusSound[50];
char topicREQfreq[50];
char topicREQKalibrasi[50];
char topicREQLogFile[50];
char topicRSPSoundStatus[50];
char topicRSPSoundAlarm[50];
char topicRSPSoundAlarmCnt[50];
char topicRSPSoundStatusVal[50];
char topicRSPSignal[50];
char topicRSPStatus[50];
char topicRSPResponse[50];
// TOPIC MQTT VAR

// MECHANIC VAR
const int delaySolenoid = 400;
const int delaySolenoidCalib = 500;
// MECHANIC VAR

struct settingsVar
{
  char idDevice[10];
  char bluetoothName[20];
  char ssid[20];
  char pass[20];
  int timeStatus;
  int timeNominal;
  int freq;
  int statusBeep;
  long alarm;
};
settingsVar configVar;

// BT VAR
char *sPtr[20];
char *strData = NULL; // this is allocated in separate and needs to be free( ) eventually
// BT VAR

char statusFileSpiffs[] = "/status.jpg";
char nominalFileSpiffs[] = "/Nominal.jpg";
char saldoFileSpiffs[] = "/saldo.jpg";
char logFileNowName[] = "/logNow.txt";
char logFileDailyName[] = "/logPrev.txt";

const String saldoEndpoint = "/saldo/response/?";
const String saldoBody = "\"imgBalance\"";
const String saldoFileName = "\"imgBalance.jpg\"";

// VARIABLE SOUND DETECTION
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
static unsigned int alamSoundVar = 0;
static unsigned int sounditronGagalKantor = 0;
static unsigned int soundValidasi = 0;
static unsigned long ts = millis();
static unsigned long last = micros();
static unsigned int sum = 0;
static unsigned int mn = 9999;
static unsigned int mx = 0;
static unsigned int cnts = 0;
static unsigned long lastTrigger[2] = {0, 0};
float loudness;
int soundValidasiCnt;
int soundValidasiCnts;

boolean detectShort = false;
boolean detectLong = false;
int detectLongCnt;
int detectShortCnt;
int cntDetected;
int cntLoss;
unsigned int SoundPeak;
bool prevAlarmStatus = false;
bool AlarmStatus = false;
bool timeAlarm = false;
int counterAlarm = 0;
int timeoutUpload = 10000;

bool bolprevAlarmSound;
bool bolnowAlarmSound;
bool bolnexAlarmSound;
int prevValidSound;
int nowValidSound;
bool bolprevValidSound;
bool bolnowValidSound;
bool bolnexValidSound;
bool outValidSound;
int counterValidasiSound;
int beginCnt;
int lastCnt;
int counterBeep;
int valueInterval;
// VARIABLE SOUND DETECTION

int cnt = 0;
Timeout TimerTimeoutAlarm;
Timeout TimerNotifAlarm;

enum class Status : uint8_t
{
  NONE,
  IDLE,
  WORKING,
  GET_KWH,
  PROCCESS_TOKEN,
  PROCCESS_TOKEN_RECORD_SOUND,
  PROCCESS_TOKEN_CAPTURE_STATUS,
  PROCCESS_TOKEN_CAPTURE_NOMINAL,
  PROCCESS_TOKEN_CAPTURE_SALDO,
  NOTIF_LOW_SALDO
};

enum class TopupStatus : uint8_t
{
  NONE,
  IDLE,
  SOLENOID,
  UPLOAD_STATUS,
  UPLOAD_NOMINAL,
  UPLOAD_SALDO
};

Status status = Status::NONE;
TopupStatus statTopup = TopupStatus::NONE;

// PREDECESSOR
void WiFiBegin();
void WiFiInit();
void logFile_i(char *msg1, char *msg2, const char *function, int line /* , char *dest */);
void dumpSysLog();
void MqttPublishResponse(String msg);
void timeToCharLog(struct tm *tm, suseconds_t usec, char *dest);
void timeToCharFile(struct tm *tm, char *dest);
bool CreatingFile(char *filename);
bool RemovingFile(char *filename);
void DuplicatingFile(char *sourceFilename, char *destFilename);
// PREDECESSOR

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

bool SettingsInit()
{
  int response;
  settings.readSettings(configFile);
  strcpy(configVar.idDevice, settings.getChar("device.id"));
  Serial.println("**BOOT**");
  Serial.println("**SETTING INIT START**");
  Serial.printf("SN DEVICE: %s\n", configVar.idDevice);
  Serial.printf("BT NAME: %s\n", settings.getChar("bluetooth.name"));

  if (strlen(settings.getChar("bluetooth.name")) == 0)
  {
    Serial.printf("BT NAME: NULL\n", settings.getChar("bluetooth.name"));
    response = settings.setChar("bluetooth.name", strcat(productName, configVar.idDevice));
    settings.writeSettings(configFile);
    Serial.println("**RESTART**");
    ESP.restart();
  }
  else
    strcpy(configVar.bluetoothName, settings.getChar("bluetooth.name"));

  strcpy(configVar.ssid, settings.getChar("wifi.ssid"));
  strcpy(configVar.pass, settings.getChar("wifi.pass"));
  configVar.timeStatus = settings.getInt("kwh.timeStatus");
  configVar.timeNominal = settings.getInt("kwh.timeNominal");
  configVar.freq = settings.getInt("kwh.freq");
  configVar.statusBeep = settings.getInt("kwh.statusbeep");
  configVar.alarm = settings.getLong("kwh.alarm");

  Serial.printf("WIFI SSID: %s\n", configVar.ssid);
  Serial.printf("WIFI PASS: %s\n", configVar.pass);
  Serial.printf("KWH TIME STATUS: %d\n", configVar.timeStatus);
  Serial.printf("KWH TIME NOMINAL: %d\n", configVar.timeNominal);
  Serial.printf("KWH FREQ: %d\n", configVar.freq);
  Serial.printf("KWH STATUS BEEP: %d\n", configVar.statusBeep);
  Serial.printf("KWH ALARM: %d\n", configVar.alarm);

  Serial.println("**SETTING INIT COMPLETE**");
  return true;
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

bool SettingsWifiSave(char *ssid, char *pass)
{
  settings.setChar("wifi.ssid", ssid);
  settings.setChar("wifi.pass", pass);

  Serial.println("---------- Save settings ----------");
  settings.writeSettings("/config.json");
  Serial.println("---------- Read settings from file ----------");
  settings.readSettings("/config.json");
  Serial.println("---------- New settings content ----------");

  return true;
}

bool SettingsSaveMqttSetting(int timeStatus, int timeNominal, int freq, long alarm, int statusBeep)
{
  // settings.setChar("wifi.ssid", wifissidbuff);
  // settings.setChar("wifi.pass", wifipassbuff);
  settings.setInt("kwh.timeStatus", timeStatus);
  settings.setInt("kwh.timeNominal", timeNominal);
  settings.setInt("kwh.freq", freq);
  settings.setLong("kwh.alarm", alarm);
  settings.setInt("kwh.statusbeep", statusBeep);

  Serial.println("---------- Save settings ----------");
  settings.writeSettings("/config.json");
  Serial.println("---------- Read settings from file ----------");
  settings.readSettings("/config.json");
  Serial.println("---------- New settings content ----------");

  return true;
}

bool SettingsSaveAll(char *ssid, char *pass, int time1, int time2, int freq, long alarm, int statusbeep)
{
  int returnVal;
  settings.setChar("wifi.ssid", ssid);
  settings.setChar("wifi.pass", pass);
  settings.setInt("kwh.timeStatus", time1);
  settings.setInt("kwh.timeNominal", time2);
  settings.setInt("kwh.freq", freq);
  settings.setLong("kwh.alarm", alarm);
  settings.setInt("kwh.statusbeep", statusbeep);

  if (settings.writeSettings(configFile) == 0)
  {
    Serial.println("--SAVING CONFIG SUCCESS--");
    returnVal = SM_SUCCESS;
  }
  else
  {
    Serial.println("--SAVING CONFIG SUCCESS--");
    returnVal = SM_SUCCESS;
  }

  if (settings.readSettings(configFile) == 0)
  {
    Serial.println("--LOAD CONFIG ERROR--");
    returnVal = SM_ERROR;
  }
  else
  {
    Serial.println("--SAVING CONFIG ERROR--");
    returnVal = SM_ERROR;
  }

  return returnVal;
}

void BluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  char BTBuffer[50];
  char ssidBuffer[20];
  char passBuffer[20];
  int kwhstat;
  int kwhnomi;
  int kwhfreq;
  int kwhstatusbeep;
  long kwhalarm;
  char respBTBuffer[100];

  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("BT Client Connected");
  }

  if (event == ESP_SPP_DATA_IND_EVT)
  {
    String btReceive = SerialBT.readString();
    Serial.println(btReceive);
    if (btReceive.length() > 5)
    {
      int N = WiFiFormParsing(btReceive, sPtr, 20, &strData);
      if (N > 2)
      {
        strcpy(ssidBuffer, sPtr[0]);
        strcpy(passBuffer, sPtr[1]);
        Serial.printf("DATA BT: ssid=%s pass=%s kwhstat=%s kwhnomi=%s kwhfreq=%s kwhstatusbeep=%s kwhstatusbeep=%s\n",
                      sPtr[0], sPtr[1], sPtr[2], sPtr[3], sPtr[5], sPtr[6], sPtr[7]);
        kwhstat = atoi(sPtr[2]);
        kwhnomi = atoi(sPtr[3]);
        kwhfreq = atoi(sPtr[6]);
        kwhstatusbeep = atoi(sPtr[5]);
        kwhalarm = atoi(sPtr[7]) * 60000;

        if (strcmp(configVar.ssid, ssidBuffer) == 0 and strcmp(configVar.pass, passBuffer) == 0)
        {
          Serial.println("SAVE WIFI SAME --> NOT RESTART");
          SettingsSaveAll(ssidBuffer, passBuffer, kwhstat, kwhnomi, kwhfreq, kwhalarm, kwhstatusbeep);
          SettingsInit();
        }

        else
        {
          Serial.println("SAVE WIFI CHANGE --> RESTART");
          SettingsSaveAll(ssidBuffer, passBuffer, kwhstat, kwhnomi, kwhfreq, kwhalarm, kwhstatusbeep);
          SettingsInit();
          // WifiInit();
          ESP.restart();
        }
      }
    }
    else if (btReceive.length() < 5 and strcmp(btReceive.c_str(), "get") == 0)
    {
      sprintf(respBTBuffer, "ssid=%s pass=%s kwhstat=%d kwhnomi=%d kwhfreq=%d kwhstatusbeep=%d kwhalarm=%d",
              configVar.ssid, configVar.pass, configVar.timeStatus, configVar.timeNominal, configVar.freq, configVar.statusBeep, configVar.alarm);
      Serial.print("SEND EXISTING DATA -> ");
      Serial.println(respBTBuffer);
      SerialBT.println(respBTBuffer);
      SerialBT.flush();
      // free(respBTBuffer);
    }
  }
}

void BluetoothInit()
{
  SerialBT.register_callback(BluetoothCallback);
  SerialBT.begin(configVar.bluetoothName);
}

// Wi-Fi events
void _wifi_event(WiFiEvent_t event)
{
  switch (event)
  {
  case SYSTEM_EVENT_WIFI_READY:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi interface ready\n"));
    break;
  case SYSTEM_EVENT_SCAN_DONE:
    // Serial.print(PSTR("[Wi-Fi] Event: Completed scan for access points\n"));
    break;
  case SYSTEM_EVENT_STA_START:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi client started\n"));
    break;
  case SYSTEM_EVENT_STA_STOP:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi clients stopped\n"));
    break;
  case SYSTEM_EVENT_STA_CONNECTED:
    // Serial.printf(PSTR("[Wi-Fi] Event: Connected to access point: %s \n"), WiFi.localIP().toString().c_str());
    // wifi_connect_task.disable();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    // Serial.print(PSTR("[Wi-Fi] Event: Not connected to Wi-Fi network\n"));
    // wifi_connect_task.enable();
    Serial.println("WiFi Disconnected. Enabling WiFi autoconnect");
    WiFi.setAutoReconnect(true);
    WiFiBegin();
    break;
  case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    // Serial.print(PSTR("[Wi-Fi] Event: Authentication mode of access point has changed\n"));
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    // Serial.printf(PSTR("[Wi-Fi] Event: Obtained IP address: %s\n"), WiFi.localIP().toString().c_str());
    // wifi_watchdog_task.enable();
    break;
  case SYSTEM_EVENT_STA_LOST_IP:
    // Serial.print(PSTR("[Wi-Fi] Event: Lost IP address and IP address is reset to 0\n"));
    break;
  case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi Protected Setup (WPS): succeeded in enrollee mode\n"));
    break;
  case SYSTEM_EVENT_STA_WPS_ER_FAILED:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi Protected Setup (WPS): failed in enrollee mode\n"));
    break;
  case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi Protected Setup (WPS): timeout in enrollee mode\n"));
    break;
  case SYSTEM_EVENT_STA_WPS_ER_PIN:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi Protected Setup (WPS): pin code in enrollee mode\n"));
    break;
  case SYSTEM_EVENT_AP_START:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi access point started\n"));
    break;
  case SYSTEM_EVENT_AP_STOP:
    // Serial.print(PSTR("[Wi-Fi] Event: Wi-Fi access point  stopped\n"));
    break;
  case SYSTEM_EVENT_AP_STACONNECTED:
    // Serial.print(PSTR("[Wi-Fi] Event: Client connected\n"));
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    // Serial.print(PSTR("[Wi-Fi] Event: Client disconnected\n"));
    break;
  case SYSTEM_EVENT_AP_STAIPASSIGNED:
    // Serial.print(PSTR("[Wi-Fi] Event: Assigned IP address to client\n"));
    break;
  case SYSTEM_EVENT_AP_PROBEREQRECVED:
    // Serial.print(PSTR("[Wi-Fi] Event: Received probe request\n"));
    break;
  case SYSTEM_EVENT_GOT_IP6:
    // Serial.print(PSTR("[Wi-Fi] Event: IPv6 is preferred\n"));
    break;
  case SYSTEM_EVENT_ETH_START:
    // Serial.print(PSTR("[Wi-Fi] Event: SYSTEM_EVENT_ETH_START\n"));
    break;
  case SYSTEM_EVENT_ETH_STOP:
    // Serial.print(PSTR("[Wi-Fi] Event: SYSTEM_EVENT_ETH_STOP\n"));
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
    // Serial.print(PSTR("[Wi-Fi] Event: SYSTEM_EVENT_ETH_CONNECTED\n"));
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
    // Serial.print(PSTR("[Wi-Fi] Event: SYSTEM_EVENT_ETH_DISCONNECTED\n"));
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    // Serial.print(PSTR("[Wi-Fi] Event: SYSTEM_EVENT_ETH_GOT_IP\n"));
    break;
  case SYSTEM_EVENT_MAX:
    break;
  }
}

void WiFiBegin()
{
  WiFi.disconnect();
  if (strcmp(configVar.ssid, "NULL") == 0)
    WiFi.begin(configVar.ssid, NULL);
  else
    WiFi.begin(configVar.ssid, configVar.pass);
  return;

  uint8_t result = WiFi.waitForConnectResult();
  if (result == WL_NO_SSID_AVAIL || result == WL_CONNECT_FAILED)
  {
    Serial.println(PSTR("[Wi-Fi] Status: Could not connect to Wi-Fi AP"));
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

void MechanicInit()
{
  Wire.begin(PIN_MCP_SDA, PIN_MCP_SCL);
  mcp.begin(&Wire);
  for (int i = 0; i < 16; i++)
  {
    mcp.pinMode(i, OUTPUT); // MCP23017 pins are set for inputs
  }
  Serial.println("MechanicInit()...Successful");
}

bool MechanicTyping(int pin)
{
  mcp.digitalWrite(pin, HIGH);
  delay(150);
  mcp.digitalWrite(pin, LOW);
  return true;
}

void MechanicEnter()
{
  MechanicTyping(11);
}

void MechanicBackSpace()
{
  MechanicTyping(9);
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

String headerNominal(char *id, char *token, size_t length)
{
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/nominal/?sn_device=" + String(id) + "&token=" + String(token) + " HTTP/1.1\r\n";
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

String headerStatusToken(String status, char *token, char *sn, size_t length)
{
  String data;
  // NEW /topup/response/status/?sn_device=" + String(sn) +"&token="+ token +"&status=" + status + " HTTP/1.1\r\n";
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/status/?sn_device=" + String(sn) + "&token=" + String(token) + "&status=" + status + " HTTP/1.1\r\n";
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

String body_Kalibrasi()
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  data += F("Content-Disposition: form-data; name=\"imgCalibrate\"; filename=\"picture.jpg\"\r\n");
  data += F("Content-Type: image/jpeg\r\n");
  data += F("\r\n");

  return (data);
}

String headerKalibrasi(char *part, size_t length)
{
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /calibrate/upload/?sn_device=" + String(configVar.idDevice) + "&part=" + String(part) + " HTTP/1.1\r\n";
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

String body_LogFile()
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  data += F("Content-Disposition: form-data; name=\"logFile\"; filename=\"log.txt\"\r\n");
  data += F("Content-Type: text/plain\r\n");
  data += F("\r\n");

  return (data);
}

String headerLogFile(char *tgl, size_t length)
{
  String data;
  // /logdevice/upload/?sn=007&tgl=22%2F04%2F07'
  data = "POST /logdevice/upload/?sn=" + String(configVar.idDevice) + "&tgl=" + String(tgl) + " HTTP/1.1\r\n";
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

String headerNotifSaldoImage(char *sn, size_t length)
{
  // POST /notif/lowsaldo/007 HTTP/1.1
  // Accept: */*
  // User-Agent: Thunder Client (https://www.thunderclient.com)
  // Content-Type: multipart/form-data; boundary=---011000010111000001101001
  // Host: 203.194.112.238:3300

  String serialnum = String(sn);
  String data;
  data = "POST /notif/lowsaldo/image/?sn_device=" + serialnum + " HTTP/1.1\r\n";
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

String body_NotifLowSaldoImage()
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  data += F("Content-Disposition: form-data; name=\"imgBalance\"; filename=\"picture.jpg\"\r\n");
  data += F("Content-Type: image/jpeg\r\n");
  data += F("\r\n");

  return (data);
}

String sendImageNominal(char *token, uint8_t *data_pic, size_t size_pic)
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
    String headerTxt = headerNominal("008", received_payload, allLen);

    if (!postClient.connect(SERVER.c_str(), PORT))
    {
      return ("connection failed");
    }

    postClient.print(headerTxt + bodyNominal);
    // Serial.print(headerTxt + bodyNominal);

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
    delay(20);
    long tOut = millis() + 10000;
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
  {
    if (postClient.connect(SERVER.c_str(), PORT))
    {

      String serverRes;
      String bodyNominal = body_Nominal();
      WiFiClient postClient;
      String bodyEnds = String("--") + BOUNDARY + String("--\r\n");
      size_t allLen = bodyNominal.length() + size_pic + bodyEnds.length();
      String headerTxt = headerNominal((char *)configVar.idDevice, received_payload, allLen);

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
      return String("ELSE UPLOADED");
    }
    else
      return String("NOT UPLOADED");
  }
}

String sendImageNominal(char *filename, char *token)
{

  String serverRes;
  String getBody;
  WiFiClient postClient;

  File nominalFile;
  nominalFile = fileSys.open(filename); // change to your file name
  size_t filesize = nominalFile.size();

  String bodyNominal = body_Nominal();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodyNominal.length() + filesize + bodyEnd.length();
  String headerTxt = headerNominal((char *)configVar.idDevice, token, allLen);
  Serial.println("Uploading Nominal...");

  if (postClient.connect(SERVER.c_str(), PORT))
  {
    Serial.println("...Connected Server");
    postClient.print(headerTxt + bodyNominal);

    const size_t bufferSize = 256;
    uint8_t buffer[bufferSize];

    while (nominalFile.available() /*and !millis() > timenow*/)
    {
      size_t n = nominalFile.readBytes((char *)buffer, bufferSize);
      // Serial.println(n);
      if (n != 0)
      {
        postClient.write(buffer, n);
        postClient.flush();
        // Serial.println(millis());
      }
      else
      {
        Serial.println(F("Buffer was empty"));
        break;
      }
    }

    // while (nominalFile.available())
    //   postClient.write(nominalFile.read());

    postClient.print("\r\n" + bodyEnd);
    nominalFile.close();
    long tOut = millis() + 10000;
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

    // return String("UPLOADED");
  }
  else
  {
    return String("ELSE UPLOADED");
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
  Serial.println("Uploading Saldo ...");
  if (postClient.connect(SERVER.c_str(), PORT))
  {
    Serial.println("...Connected Server");
    postClient.print(headerTxt + bodySaldo);
    // Serial.print(headerTxt + bodySaldo);
    postClient.write(data_pic, size_pic);

    postClient.print("\r\n" + bodyEnd);
    // Serial.print("\r\n" + bodyEnd);
    // postClient.stop();
    delay(20);
    long tOut = millis() + 10000;
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
  {

    return String("NOT UPLOADED");
  }
}

String sendImageSaldo(char *filename, char *token)
{
  String serverRes;
  WiFiClient postClient;

  File saldoFile;
  saldoFile = fileSys.open(filename); // change to your file name
  size_t filesize = saldoFile.size();

  String bodySaldo = body_Saldo();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySaldo.length() + filesize + bodyEnd.length();
  String headerTxt = headerSaldo(token, allLen);
  Serial.println("Upload Saldo...");
  if (postClient.connect(SERVER.c_str(), PORT))
  {
    Serial.println("...Connected Server");
    postClient.print(headerTxt + bodySaldo);

    const size_t bufferSize = 256;
    uint8_t buffer[bufferSize];

    while (saldoFile.available() /*and !millis() > timenow*/)
    {
      size_t n = saldoFile.readBytes((char *)buffer, bufferSize);
      // Serial.println(n);
      if (n != 0)
      {
        postClient.write(buffer, n);
        postClient.flush();
        // Serial.println(millis());
      }
      else
      {
        Serial.println(F("Buffer was empty"));
        break;
      }
    }

    // while (saldoFile.available())
    //   postClient.write(saldoFile.read());

    postClient.print("\r\n" + bodyEnd);
    saldoFile.close();

    delay(20);
    long tOut = millis() + 10000;
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
    // postClient.stop();
    // return String("UPLOADED");
  }
  else
  {

    return String("ELSE UPLOADED");
  }
}

String sendImageStatusToken(String status, char *token, char *sn, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  WiFiClient postClient;
  String bodySatus = body_StatusToken();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySatus.length() + size_pic + bodyEnd.length();
  String headerTxt = headerStatusToken(status, token, sn, allLen);

  if (postClient.connect(SERVER.c_str(), PORT))
  {
    postClient.print(headerTxt + bodySatus);
    postClient.write(data_pic, size_pic);
    postClient.print("\r\n" + bodyEnd);
    delay(20);
    long tOut = millis() + 10000;
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
    return serverRes;
  }
  else
  {
    if (postClient.connect(SERVER.c_str(), PORT))
    {
      postClient.print(headerTxt + bodySatus);
      postClient.write(data_pic, size_pic);
      return String("ELSE UPLOADED");
    }
    else
      return String("NOT UPLOADED");
  }
}

String sendImageStatusToken(char *filename, String status, char *token, char *sn)
{
  String serverRes;
  WiFiClient postClient;
  File statusFile;

  statusFile = fileSys.open(filename); // change to your file name
  size_t filesize = statusFile.size();

  String bodySatus = body_StatusToken();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySatus.length() + filesize + bodyEnd.length();
  String headerTxt = headerStatusToken(status, token, sn, allLen);
  Serial.println("Upload Status...");
  if (postClient.connect(SERVER.c_str(), PORT))
  {
    Serial.println("...Connected Server");
    postClient.print(headerTxt + bodySatus);

    const size_t bufferSize = 256;
    uint8_t buffer[bufferSize];

    while (statusFile.available() /*and !millis() > timenow*/)
    {
      size_t n = statusFile.readBytes((char *)buffer, bufferSize);
      // Serial.println(n);
      if (n != 0)
      {
        postClient.write(buffer, n);
        postClient.flush();
        // Serial.println(millis());
      }
      else
      {
        Serial.println(F("Buffer was empty"));
        break;
      }
    }

    // while (statusFile.available())
    // {
    //   postClient.write(statusFile.read());
    // }
    postClient.print("\r\n" + bodyEnd);
    statusFile.close();
    // delay(20);
    long tOut = millis() + 10000;
    while (tOut > millis())
    {
      Serial.print(".");
      delay(100);
      if (postClient.available())
      {
        serverRes = postClient.readStringUntil('\r');
        return (serverRes);
      }
    }
    // postClient.stop();
    return serverRes;
    // return String("UPLOADED");
  }
  else
  {
    if (postClient.connect(SERVER.c_str(), PORT))
    {
      postClient.print(headerTxt + bodySatus);
      Serial.print(headerTxt + bodySatus);
      while (statusFile.available())
        postClient.write(statusFile.read());

      postClient.print("\r\n" + bodyEnd);
      Serial.print("\r\n" + bodyEnd);
      statusFile.close();
    }
    return String("ELSE UPLOADED");
  }
}

String sendImageViewKWH(char *sn)
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

  Serial.println("Connecting to server: " + SERVER);

  if (postClient.connect(SERVER.c_str(), PORT))
  {
    Serial.println("Connection successful!");
    String head = "--MGI\r\nContent-Disposition: form-data; name=" + saldoBody + "; filename=" + saldoFileName + "\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--MGI--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    postClient.println("POST " + saldoEndpoint + "&sn_device=" + sn + " HTTP/1.1");
    Serial.println("POST " + saldoEndpoint + "&sn_device=" + sn + " HTTP/1.1");

    postClient.println("Host: " + SERVER);
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
    // postClient.stop();
    //   int timoutTimer = 10000;
    //   long startTimer = millis();
    //   boolean state = false;

    //   while ((startTimer + timoutTimer) > millis())
    //   {
    //     Serial.print(".");
    //     delay(100);
    //     while (postClient.available())
    //     {
    //       char c = postClient.read();
    //       if (c == '\n')
    //       {
    //         if (getAll.length() == 0)
    //         {
    //           state = true;
    //         }
    //         getAll = "";
    //       }
    //       else if (c != '\r')
    //       {
    //         getAll += String(c);
    //       }
    //       if (state == true)
    //       {
    //         getBody += String(c);
    //       }
    //       startTimer = millis();
    //     }
    //     if (getBody.length() > 0)
    //     {
    //       break;
    //     }
    //   }
    //   Serial.println();
    //   // postClient.stop();
    //   Serial.println(getBody);
    // }
    // else
    // {
    //   getBody = "Connection to " + serverName + " failed.";
    //   Serial.println(getBody);
  }
  return String("UPLOADED");
}

String sendImageKalibrasi(char *part, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  WiFiClient postClient;
  String bodyKalibrasi = body_Kalibrasi();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodyKalibrasi.length() + size_pic + bodyEnd.length();
  String headerTxt = headerKalibrasi(part, allLen);

  if (!postClient.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }
  Serial.printf("Upload Image Kalibrasi %s\n", part);
  postClient.print(headerTxt + bodyKalibrasi);
  // Serial.print(headerTxt + bodyKalibrasi);
  postClient.write(data_pic, size_pic);

  postClient.print("\r\n" + bodyEnd);
  // Serial.print("\r\n" + bodyEnd);
  // delay(20);
  // long tOut = millis() + 10000;
  // while (tOut > millis())
  // {
  //   Serial.print(".");
  //   delay(100);
  //   if (postClient.available())
  //   {
  //     serverRes = postClient.readStringUntil('\r');
  //     // return (serverRes);
  //   }
  //   else
  //   {
  //     serverRes = "NOT RESPONSE";
  //     // return (serverRes);
  //   }
  // }
  return serverRes;
}

String LogUpload(char *filename, char *tgl)
{
  String getAll;
  String getBody;
  WiFiClient postClient;

  String bodyLog = body_LogFile();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  File logFile;

  logFile = SPIFFS.open(filename); // change to your file name
  size_t filesize = logFile.size();

  size_t allLen = bodyLog.length() + filesize + bodyEnd.length();
  String headerTxt = headerLogFile(tgl, allLen);
  Serial.printf("Upload Log %s %s\n", filename, tgl);
  if (!postClient.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  postClient.print(headerTxt + bodyLog);
  Serial.print(headerTxt + bodyLog);
  while (logFile.available())
    postClient.write(logFile.read());

  postClient.print("\r\n" + bodyEnd);
  Serial.print("\r\n" + bodyEnd);
  // postClient.stop();
  return ("UPLOADED");
}

String sendNotifSaldoImage(uint8_t *data_pic, size_t size_pic)
{
  String getAll;
  String getBody;
  WiFiClient postClient;

  Serial.println("Connecting to server: " + SERVER);

  if (postClient.connect(SERVER.c_str(), PORT))
  {
    String serverRes;
    String bodyNotif = body_NotifLowSaldoImage();
    WiFiClient postClient;
    String bodyEnds = String("--") + BOUNDARY + String("--\r\n");
    size_t allLen = bodyNotif.length() + size_pic + bodyEnds.length();
    String headerTxt = headerNotifSaldoImage(configVar.idDevice, allLen);

    if (!postClient.connect(SERVER.c_str(), PORT))
    {
      return ("connection failed");
    }

    postClient.print(headerTxt + bodyNotif);
    Serial.print(headerTxt + bodyNotif);

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

    long tOut = millis() + 10000;
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
  //   return String("UPLOADED");
  // }
  // else
  //   return String("NOT CONNECT");
}

void GetKwhProcess()
{
  if (received_msg and strcmp(received_topic, topicREQView) == 0 and strcmp(received_payload, "view") == 0)
  {
    status = Status::GET_KWH;
    MqttPublishResponse("view");
#ifdef USE_LOG
    logFile_i("Retrieve Mqtt Command GetKWH", "", __FUNCTION__, __LINE__);
#endif

    Serial.println("GET_KWH");
#ifdef USE_SOLENOID
    MechanicBackSpace();
#endif
    Serial.println(sendImageViewKWH((char *)configVar.idDevice));
#ifdef USE_LOG
    logFile_i("Capture and Upload Photo GetKWH", "", __FUNCTION__, __LINE__);
#endif
    received_msg = false;
    status = Status::IDLE;
  }
}

void GetLogFile(/*char *token*/)
{
  if (received_msg and strcmp(received_topic, topicREQLogFile) == 0 and strcmp(received_payload, "now") == 0)
  {
    status = Status::WORKING;
    MqttPublishResponse("log");
#ifdef USE_LOG
    logFile_i("Receive Log Now Command", "", __FUNCTION__, __LINE__);
#endif
    char logNowDate[20];
    // MqttPublishStatus("UPLOAD LOG FILE NOW");
    struct tm *tm = pftime::localtime(nullptr);
    timeToCharFile(tm, logNowDate);
    LogUpload(logFileNowName, logNowDate);
#ifdef USE_LOG
    logFile_i("Upload Log Now", "", __FUNCTION__, __LINE__);
#endif
    received_msg = false;
    status = Status::IDLE;
  }

  if (received_msg and strcmp(received_topic, topicREQLogFile) == 0 and strcmp(received_payload, "hourly") == 0)
  {
    status = Status::WORKING;
    MqttPublishResponse("log");
#ifdef USE_LOG
    logFile_i("Receive Log Daily Command", "", __FUNCTION__, __LINE__);
#endif
    char logDailyDate[20];
    // MqttPublishStatus("UPLOAD LOG FILE HARIAN");
    struct tm *tm = pftime::localtime(nullptr);
    timeToCharFile(tm, logDailyDate);
    DuplicatingFile(logFileNowName, logFileDailyName);
    LogUpload(logFileDailyName, logDailyDate);
#ifdef USE_LOG
    logFile_i("Upload Log Daily", "", __FUNCTION__, __LINE__);
#endif
    RemovingFile(logFileDailyName);
    received_msg = false;
    status = Status::IDLE;
  }
}

void CaptureAndSendNotif()
{
#ifdef USE_SOLENOID
  MechanicBackSpace();
#endif
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb); // dispose the buffered image
  fb = NULL;                // reset to capture errors
  fb = esp_camera_fb_get(); // get fresh image
                            // OutputSoundValidasi(SoundDetectValidasi());
  String res;
  res = sendNotifSaldoImage(fb->buf, fb->len);
  Serial.println(res);
  esp_camera_fb_return(fb);
}

void MqttCallback(char *topic, byte *payload, unsigned int length)
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
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
  while (!mqtt.connected())
  {
    String clientId = "ESP8266Client-";
    String rand;
    Serial.print("Attempting MQTT connection...");
    // for (uint8_t i = 0; i < 8; i++)
    // {
    //   rand[i] = alphanum[random(62)];
    // }
    // rand[8] = '\0';
    clientId = "TokenOnline_" + String(configVar.idDevice) + String(random(0xffff), HEX);
    // clientId = "huhuhu";
    // clientId += String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str()))
    {
      Serial.println("connected");
      mqtt.subscribe(topicREQAlarm);
      mqtt.subscribe(topicREQtimeStatus);
      mqtt.subscribe(topicREQtimeNomi);
      mqtt.subscribe(topicREQstatusSound);
      mqtt.subscribe(topicREQfreq);
      mqtt.subscribe(topicREQToken);
      mqtt.subscribe(topicREQView);
      mqtt.subscribe(topicREQKalibrasi);
      mqtt.subscribe(topicREQLogFile);
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

void MqttTopicInit()
{
  sprintf(topicRSPSignal, "%s/%s/%s/%s", mqttChannel, "respond", configVar.idDevice, "signal");
  sprintf(topicRSPStatus, "%s/%s/%s/%s", mqttChannel, "respond", configVar.idDevice, "debug");
  sprintf(topicRSPResponse, "%s/%s/%s/%s", mqttChannel, "respond", configVar.idDevice, "response");

  sprintf(topicRSPSoundStatus, "%s/%s/%s/%s", mqttChannel, "respond", configVar.idDevice, "sound/status");
  sprintf(topicRSPSoundStatusVal, "%s/%s/%s/%s", mqttChannel, "respond", configVar.idDevice, "sound/statusval");
  sprintf(topicRSPSoundAlarm, "%s/%s/%s/%s", mqttChannel, "respond", configVar.idDevice, "sound/alarm");
  sprintf(topicRSPSoundAlarmCnt, "%s/%s/%s/%s", mqttChannel, "respond", configVar.idDevice, "sound/alarmcnt");

  sprintf(topicREQToken, "%s/%s/%s", mqttChannel, "request", configVar.idDevice);
  Serial.println(topicREQToken);
  sprintf(topicREQView, "%s/%s/%s/%s", mqttChannel, "request", "view", configVar.idDevice);
  sprintf(topicREQKalibrasi, "%s/%s/%s/%s", mqttChannel, "request", configVar.idDevice, "kalibrasi");
  sprintf(topicREQLogFile, "%s/%s/%s/%s", mqttChannel, "request", configVar.idDevice, "log/");

  sprintf(topicREQAlarm, "%s/%s/%s/%s", mqttChannel, "request", configVar.idDevice, "alarm");
  sprintf(topicREQtimeStatus, "%s/%s/%s/%s", mqttChannel, "request", configVar.idDevice, "delayStatus");
  sprintf(topicREQtimeNomi, "%s/%s/%s/%s", mqttChannel, "request", configVar.idDevice, "delayNominal");
  sprintf(topicREQstatusSound, "%s/%s/%s/%s", mqttChannel, "request", configVar.idDevice, "statusSound");
  sprintf(topicREQfreq, "%s/%s/%s/%s", mqttChannel, "request", configVar.idDevice, "freqPenempatan");
}

void MqttCustomPublish(char *path, String msg)
{
  mqtt.publish(path, msg.c_str());
}

void MqttPublishResponse(String msg)
{
  MqttCustomPublish(topicRSPResponse, msg);
}

int char2int(char *array, size_t n)
{
  int number = 0;
  int mult = 1;

  n = (int)n < 0 ? -n : n; /* quick absolute value check  */

  /* for each character in array */
  while (n--)
  {
    /* if not digit or '-', check if number > 0, break or continue */
    if ((array[n] < '0' || array[n] > '9') && array[n] != '-')
    {
      if (number)
        break;
      else
        continue;
    }

    if (array[n] == '-')
    { /* if '-' if number, negate, break */
      if (number)
      {
        number = -number;
        break;
      }
    }
    else
    { /* convert digit to numeric value   */
      number += (array[n] - '0') * mult;
      mult *= 10;
    }
  }

  return number;
}

///////////////////////////////// SOUND //////////////////////////////
///////////////////////////////// SOUND //////////////////////////////
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
  loudness = calculateLoudness(energy, aweighting, OCTAVES, 1.0);
  SoundPeak = (int)floor(fft.MajorPeak());
}

bool DetectLowTokenV2()
{
  SoundLooping();
  bolprevAlarmSound = bolnowAlarmSound;
  bolnowAlarmSound = bolnexAlarmSound;
  bolnexAlarmSound = SoundPeak == configVar.freq || (SoundPeak == configVar.freq + 1 || SoundPeak == configVar.freq - 1);
  if (bolprevAlarmSound < bolnowAlarmSound and bolnowAlarmSound > bolnexAlarmSound)
  {
    TimerTimeoutAlarm.start(6000);
    prevAlarmStatus = true;
    counterAlarm++;
  }
  //  if (bolnowVal > bolnexVal)
  // {
  //   counterAlarm++;
  // }
  // else
  //   return counterAlarm = 0;
  else if (TimerTimeoutAlarm.time_over())
  {
    counterAlarm = 0;
    AlarmStatus = false;
    prevAlarmStatus = false;
    // timeAlarm = false;
  }

  if (counterAlarm > 30)
  {
    AlarmStatus = true;

    // MqttCustomPublish(topicRSPSoundAlarm, String(counter));
    // counter = 0;
    // prevAlarmStatus = false;
    // timeAlarm = false;
  }

  if (TimerNotifAlarm.time_over())
  {
    Serial.println("COUNTDOWN TIME OVER");
    if (counterAlarm == 0)
    {
      Serial.println("COUNTDOWN START , COUNTER 0");
      TimerNotifAlarm.start(configVar.alarm);
    }
    else if (counterAlarm > 12)
    {
      Serial.println("COUNTDOWN TIME OVER, TIME ALARM TRUE");
      timeAlarm = true;
    }
  }

  if (AlarmStatus and timeAlarm)
  {
    CaptureAndSendNotif();
#ifdef USE_LOG
    logFile_i("Notif Low Saldo", "", __FUNCTION__, __LINE__);
#endif
    //
    Serial.println("SEND NOTIFICATION");

    counterAlarm = 0;
    AlarmStatus = false;
    prevAlarmStatus = false;
    timeAlarm = false;
    TimerNotifAlarm.start(configVar.alarm);
  }

  return false;
}

bool SoundDetectAlarm()
{
  SoundLooping();

  // bool detectedShortItronFast2 = detectFrequency(&itronShortFast2, 6, SoundPeak, 186, 190, true);
  bool detectedShortItronFast = detectFrequency(&alamSoundVar, 6, SoundPeak, configVar.freq, 190, true);

  // Serial.printf("%d %d %d %f\n", SoundPeak, detectedShortItronFast, detectShortCnt, loudness);
  if (detectedShortItronFast and (detectShortCnt > 20 /*25 and detectShortCnt < 35)*/))
  {
    // MqttCustomPublish(topicRSPStatus, "DETECTED SHORT BEEP");
    // Serial.println("DETECTED SHORT BEEP");
    cntDetected = 0;
    detectShortCnt = 0;
    return true;
  }
  else
  {

    if (detectedShortItronFast and loudness > 20) // and !detectLong)
    {
      cntDetected++;
      detectShortCnt++;
    }
    else
    {
      cntDetected = 0;
      detectShortCnt = 0;
    }
    // MqttCustomPublish(topicRSPSoundPeak, String(SoundPeak));
    return false;
  }
}

bool DetectionLowToken()
{
  if (SoundDetectAlarm())
  {
    TimerTimeoutAlarm.start(6000);
    prevAlarmStatus = true;
    counterAlarm++;

    MqttCustomPublish(topicRSPSoundAlarmCnt, String(counterAlarm));
  }
  else if (TimerTimeoutAlarm.time_over())
  {
    counterAlarm = 0;
    AlarmStatus = false;
    prevAlarmStatus = false;
  }

  if (counterAlarm > 10)
  {
    AlarmStatus = true;
    prevAlarmStatus = true;
  }

  if (TimerNotifAlarm.time_over())
  {

    timeAlarm = true;
  }

  if ((AlarmStatus and timeAlarm) or (prevAlarmStatus and timeAlarm))
  {
    // CaptureAndSendNotif();
    Serial.println("SEND NOTIFICATION");
    counterAlarm = 0;
    AlarmStatus = false;
    prevAlarmStatus = false;
    timeAlarm = false;
    TimerNotifAlarm.start(configVar.alarm);
  }

  return false;
}

void CaptureToSpiffs(fs::FS &fs, String path, uint8_t *data_pic, size_t size_pic)
{
  if (!fileSys.begin())
  {
    Serial.println("SD Card Mount Failed");
    return;
  }

  Serial.printf("Picture file name: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
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

bool RemovingFile(char *filename)
{
  bool stateFile;
  if (fileSys.exists(filename))
  {
    Serial.printf("File Daily log %s exits\n", filename);
  }
  else
    return false;

  stateFile = fileSys.remove(filename);
  if (stateFile)
    Serial.printf("File Daily log %s Removed\n ", filename);
  else
    Serial.printf("File Daily log %s Cannot Removed\n", filename);
  return stateFile;
}

bool CreatingFile(char *filename)
{
  bool stateFile;
  if (fileSys.exists(filename))
  {
    Serial.printf("File log Now %s exits\n", filename);
    return true;
  }
  else
  {
    File fileNew = fileSys.open(logFileNowName, FILE_WRITE, true);
    Serial.printf("File %s created\n", fileNew);
    fileNew.close();
  }
  if (fileSys.exists(filename))
  {
    Serial.printf("File log Now %s exits\n", filename);
    return true;
  }
  else
    return false;

  return stateFile;
}

void DuplicatingFile(char *sourceFilename, char *destFilename)
{
  if (fileSys.exists(destFilename))
  {
    Serial.printf("File %s exits\n", destFilename);
  }
  else
  {
    File file = fileSys.open(destFilename, FILE_WRITE, true);
    Serial.printf("File %s created\n", destFilename);
    file.close();
  }
  File filenow = fileSys.open(sourceFilename, "r");
  File fileprev = fileSys.open(destFilename, FILE_WRITE);
  if (!filenow and !fileprev)
  {
    Serial.printf("File %s and %s open failed\n", sourceFilename, destFilename);
  }
  int fileSize = (filenow.size());
  Serial.printf("DUPLICATING %s to %s\n", sourceFilename, destFilename);
  while (filenow.available())
    fileprev.write(filenow.read());

  filenow.close();
  fileprev.close();
  Serial.printf("DUPLICATING %s to %s SUCCESS\n", sourceFilename, destFilename);
}

// timetocharLog -> [YYYY/MM/DD HH:MM:SS.MSMSMS]
void timeToCharLog(struct tm *tm, suseconds_t usec, char *dest)
{
  sprintf(dest, "[%04d/%02d/%02d %02d:%02d:%02d]",
          tm->tm_year + 1900,
          tm->tm_mon + 1,
          tm->tm_mday,
          tm->tm_hour,
          tm->tm_min,
          tm->tm_sec);
}

// timetocharFile -> YYYY/MM/DD
void timeToCharFile(struct tm *tm, char *dest)
{
  sprintf(dest, "%04d-%02d-%02d",
          tm->tm_year + 1900,
          tm->tm_mon + 1,
          tm->tm_mday);
}

void RequestKalibrasiSolenoids()
{
  if (received_msg and strcmp(received_topic, topicREQKalibrasi) == 0 /* and strcmp(received_payload, "calibrate") == 0 */)
  {
    status = Status::WORKING;
    MqttPublishResponse("calibrate");
    // MqttPublishStatus("KALIBRASI SELENOID 1 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 1");
#ifdef USE_SOLENOID
    MechanicTyping(0);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 2 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 2");
#ifdef USE_SOLENOID
    MechanicTyping(1);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 3 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 3");
#ifdef USE_SOLENOID
    MechanicTyping(2);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 4 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 4");
#ifdef USE_SOLENOID
    MechanicTyping(3);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 5 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 5");
#ifdef USE_SOLENOID
    MechanicTyping(4);
#endif
    delay(delaySolenoidCalib);

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
      res = sendImageKalibrasi("1", fb->buf, fb->len);
      Serial.println(res);
      esp_camera_fb_return(fb);
    }

    // MqttPublishStatus("KALIBRASI SELENOID 6 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 6");
#ifdef USE_SOLENOID
    MechanicTyping(5);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 7 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 7");
#ifdef USE_SOLENOID
    MechanicTyping(6);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 8 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 8");
#ifdef USE_SOLENOID
    MechanicTyping(7);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 9 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 9");
#ifdef USE_SOLENOID
    MechanicTyping(8);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 0 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 0");
#ifdef USE_SOLENOID
    MechanicTyping(10);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishStatus("KALIBRASI SELENOID 0 ON");
    // MqttPublishResponse("Work: Kalibrasi Solenoid 0");
#ifdef USE_SOLENOID
    MechanicTyping(10);
#endif
    delay(delaySolenoidCalib);

    // MqttPublishResponse("Work: Kalibrasi Solenoid Backspace");
#ifdef USE_SOLENOID
    MechanicBackSpace();
#endif

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
      res = sendImageKalibrasi("2", fb->buf, fb->len);
      Serial.println(res);
      esp_camera_fb_return(fb);
    }

    // MqttPublishResponse("Work: Kalibrasi Solenoid Enter");
#ifdef USE_SOLENOID
    MechanicEnter();
#endif

    received_msg = false;
    status = Status::IDLE;
    // MqttPublishResponse("Idle");
  }
}

void ReceiveTopup()
{
  if (received_msg and strcmp(received_topic, topicREQToken) == 0 and received_length == 20)
  {
    btStop();
    status = Status::PROCCESS_TOKEN;
#ifdef USE_LOG
    logFile_i("Receive Token :", received_payload, __FUNCTION__, __LINE__);
#endif
    dumpSysLog();
    for (int i = 0; i < received_length; i++)

    {
      char chars = received_payload[i];
      Serial.printf("%d ", i);

      switch (chars)
      {
      case '0':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));

#ifdef USE_SOLENOID
        MechanicTyping(10);
#endif

        delay(delaySolenoid);
        break;

      case '1':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(0);
#endif

        delay(delaySolenoid);
        break;

      case '2':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(1);
#endif

        delay(delaySolenoid);
        break;
      case '3':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(2);
#endif

        delay(delaySolenoid);
        break;
      case '4':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(3);
#endif

        delay(delaySolenoid);
        break;
      case '5':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(4);
#endif
        delay(delaySolenoid);
        break;
      case '6':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(5);
#endif
        delay(delaySolenoid);
        break;
      case '7':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(6);
#endif
        delay(delaySolenoid);
        break;
      case '8':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(7);
#endif
        delay(delaySolenoid);
        break;
      case '9':
        // MqttPublishStatus("SELENOID " + String(chars) + " ON");
        // MqttPublishResponse("Work: Type Solenoid " + String(chars));
#ifdef USE_SOLENOID
        MechanicTyping(8);
#endif
        delay(delaySolenoid);
        break;
      default:
        break;
      }
    }
#ifdef USE_SOLENOID
    MechanicEnter();
#endif
    delay(delaySolenoid);

    camera_fb_t *fb = NULL;
    // sysLog.writef("[%d] Receive Payload : %s", millis(), received_payload);
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb); // dispose the buffered image
    fb = NULL;                // reset to capture errors
    fb = esp_camera_fb_get(); // get fresh image
    if (!fb)
    {
      ESP.restart();
    }
    else
    {
      CaptureToSpiffs(fileSys, statusFileSpiffs, fb->buf, fb->len);
    }
    esp_camera_fb_return(fb);
    // camera_fb_t *fbnominal = NULL;

    esp_camera_fb_return(fb); // dispose the buffered image
    fb = NULL;                // reset to capture errors
    fb = esp_camera_fb_get();
    if (!fb)
    {
      ESP.restart();
    }
    else
    {
      CaptureToSpiffs(fileSys, nominalFileSpiffs, fb->buf, fb->len);
    }
    esp_camera_fb_return(fb);

    camera_fb_t *fbsaldo = NULL;
    fbsaldo = esp_camera_fb_get();
    esp_camera_fb_return(fbsaldo); // dispose the buffered image
    fbsaldo = NULL;                // reset to capture errors
    fbsaldo = esp_camera_fb_get();

    if (!fbsaldo)
    {
      ESP.restart();
    }
    else
    {
      CaptureToSpiffs(fileSys, saldoFileSpiffs, fbsaldo->buf, fbsaldo->len);
    }
    esp_camera_fb_return(fbsaldo);

    Serial.println(sendImageNominal(nominalFileSpiffs, received_payload));
    yield();
    Serial.println(sendImageSaldo(saldoFileSpiffs, received_payload));
    yield();
    // Serial.println(sendImageStatusToken(statusFileSpiffs, "invalid", received_payload, (char *)configVar.idDevice));
    // yield();
    received_msg = false;
    status = Status::IDLE;
  }
}

void dumpSysLog()
{
  int seekVal = 0, len = 0;
  char cIn;

  File sl = fileSys.open(logFileNowName, "r");

  while (sl.available())
  {
    cIn = (char)sl.read();
    len++;
    seekVal++;

    Serial.print(cIn);
  }
  sl.close();

} //  dumpSysLog()

void LogInit()
{
  fileSys.begin(true);

  if (!sysLog.begin(20, 100))
  {
    Serial.println("Error opening sysLog!\r\nCreated sysLog!");
    delay(5000);
  }
  sysLog.setOutput(&Serial);
  sysLog.status();
}

void logFile_i(char *msg1, char *msg2, const char *function, int line /* , char *dest */)
{
  // filePrint.open();
  char charTime[30];
  struct tm *tm = pftime::localtime(nullptr);
  suseconds_t usec;
  timeToCharLog(tm, usec, charTime);
  Serial.println(charTime);

  int strtId = sysLog.getLastLineID();
  sysLog.writef("%s %-5s[%-5s:%d] : %s %s", charTime, "INFO", function, line, msg1, msg2);
  // sysLog.writef("zzzzzzzzzz\n");

  // filePrint.close();
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
  SettingsInit();
  BluetoothInit();
  WiFiInit();
  CameraInit();
  SoundInit();
#ifdef USE_SOLENOID
  MechanicInit();
#endif

  fileSys.begin(true);
  LogInit();
  listDir(fileSys, "/", 3);
  // fileSys.end();
  pftime::configTzTime(PSTR("WIB-7"), ntpServer);
  mqtt.setServer(mqttServer, 1883);
  mqtt.setCallback(MqttCallback);
  MqttTopicInit();
  delay(5000);
  status = Status::IDLE;
}

void loop()
{
  // SoundLooping();
  if (millis() > 10000)
    btStop();
  if (!mqtt.connected())
    MqttReconnect();
  if (status == Status::IDLE)
  {
    DetectLowTokenV2();
    RequestKalibrasiSolenoids();
    GetLogFile();
    GetKwhProcess();
  }

  mqtt.loop();
  ReceiveTopup();
}