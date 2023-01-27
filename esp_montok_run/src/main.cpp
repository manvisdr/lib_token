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
#include <Preferences.h>
#include <ElegantOTA.h>
#include "esp_camera.h"
#include "EEPROM.h"
#include <ESP32_OV5640_AF.h>

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

#include <TaskScheduler.h>

#define WIFI_CONNECT_INTERVAL 5000  // Connection retry interval in milliseconds
#define WIFI_WATCHDOG_INTERVAL 5000 // Wi-Fi watchdog interval in milliseconds

// Callback methods prototypes
void wifi_connect_cb();
void wifi_watchdog_cb();
// Tasks
Task wifi_connect_task(WIFI_CONNECT_INTERVAL, TASK_FOREVER, &wifi_connect_cb);
Task wifi_watchdog_task(WIFI_WATCHDOG_INTERVAL, TASK_FOREVER, &wifi_watchdog_cb);
// Task runner
Scheduler wifiReconRunner;
Preferences EepromStatus;

AlarmId pingTime;
AlarmId pingTimeSend;
int counterPing;
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
OV5640 ov5640 = OV5640();
IPAddress mqttServer(203, 194, 112, 238);
IPAddress customServer(203, 194, 112, 238);
Timeout timergagal;
Timeout timer;
Timeout TimeKwhAlarm;
#define ITRON
// #define HEXING
#ifdef ITRON
int delayStatus = 700;
int delayNominal = 1700;
int delaySaldo = 4000;
int freqPeak = 186; // kantor 182, pak ferdi 186
#endif
#ifdef HEXING
int delayStatus = 1100;
int delayNominal = 5200;
int delaySaldo = 11000;
int freqPeak = 117;
#endif

// #define USE_SOLENOID
#define TIMEOUT_SUARA 600

int delaySolenoid = 1000;
long validasiStartTimer;
long afterSolenoid;
long TimeKwhAlarmNow;

enum class Status : uint8_t
{
  NONE,
  IDLE,
  GET_KWH,
  PROCCESS_TOKEN,
  PROCCESS_TOKEN_RECORD_SOUND,
  PROCCESS_TOKEN_CAPTURE_STATUS,
  PROCCESS_TOKEN_CAPTURE_NOMINAL,
  PROCCESS_TOKEN_CAPTURE_SALDO,
  NOTIF_LOW_SALDO
};

Status status = Status::NONE;

bool TokenStateStatus = false;
bool TokenStateSoundRec = false;
bool TokenStateNominal = false;
bool TokenStateSaldo = false;

const char *configFile = "/config.json";
char productName[16] = "TOKEN-ONLINE_";
// SETTTING BUFF
char wifissidbuff[20];
char wifipassbuff[20];
int kwhtimestatbuff;
int kwhtimenomibuff;
int kwhfreqbuff;
int kwhstatusbeepbuff;
int kwhalarmbuff;
// SETTING BUFF

char idDevice[9] = "007";
char bluetooth_name[sizeof(productName) + sizeof(idDevice)];
const char *mqttChannel = "TokenOnline";
const char *MqttUser = "das";
const char *MqttPass = "mgi2022";

char topicREQToken[50];
char topicREQAlarm[50];
char topicREQView[50];
char topicREQtimeStatus[50];
char topicREQtimeNomi[50];
char topicREQstatusSound[50];
char topicREQfreq[50];
char topicRSPSoundStatus[50];
char topicRSPSoundPeak[50];
char topicRSPSoundCnt[50];

char topicRSPSignal[50];
char topicRSPStatus[50];

char PrevToken[21];
char NowToken[21];

long start_wifi_millis;
long wifi_timeout = 10000;
long tOut;

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
static unsigned int bell = 0;
static unsigned int hexingShort = 0;
static unsigned int hexingLong = 0;
static unsigned int itronShortFast = 0;
static unsigned int itronShortFast2 = 0;
static unsigned int itronBerhasilKantor = 0;
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

boolean detectShort = false;
boolean detectLong = false;
int detectLongCnt;
int detectShortCnt;
int cntDetected;
int cntLoss;

unsigned int SoundPeak;
// VARIABLE SOUND DETECTION

bool prevAlarmStatus = false;
bool AlarmStatus = false;
bool timeAlarm = false;

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

const int payloadSize = 100;
const int topicSize = 128;
char received_topic[topicSize];
char received_payload[payloadSize];
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

bool send = false;
int timeEnableAlarm = 60;

void MqttCustomPublish(char *path, String msg);
void MqttPublishStatus(String msg);
void WifiInit();

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
    wifi_connect_task.disable();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    // Serial.print(PSTR("[Wi-Fi] Event: Not connected to Wi-Fi network\n"));
    wifi_connect_task.enable();
    break;
  case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    // Serial.print(PSTR("[Wi-Fi] Event: Authentication mode of access point has changed\n"));
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    // Serial.printf(PSTR("[Wi-Fi] Event: Obtained IP address: %s\n"), WiFi.localIP().toString().c_str());
    wifi_watchdog_task.enable();
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

// Wi-Fi connect task
void wifi_connect_cb()
{
  wifi_connect_task.disable();

  Serial.println(PSTR("[Wi-Fi] Status: Connecting ..."));
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
    wifi_connect_task.enableDelayed(1000);
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

// Wi-Fi watchdog task
void wifi_watchdog_cb()
{
  // Serial.println(PSTR("[Watchdog] Checking Wi-Fi ..."));
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(PSTR("[Watchdog] Wi-Fi Disconnected"));
    WiFi.disconnect();
    wifi_watchdog_task.disable();
    wifi_connect_task.enableDelayed(2500);
    return;
  }
  // Serial.println(PSTR("[Watchdog] Wi-Fi is connected!"));
}

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
    // MechanicTyping(i);
    // delay(10);
  }
  // for (int i = 0; i < 16; i++)
  // {
  //   MechanicTyping(i);
  //   // delay(10);
  // }
}

void MechanicEnter()
{
  MechanicTyping(11);
}

void MechanicBackSpace()
{
  MechanicTyping(9);
}

bool SettingsInit()
{
  int response;
  settings.readSettings(configFile);
  strcpy(idDevice, settings.getChar("device.id"));
  Serial.println("**BOOT**");
  Serial.println("**SETTING INIT START**");
  Serial.printf("SN DEVICE: %s\n", idDevice);
  Serial.printf("BT NAME: %s\n", settings.getChar("bluetooth.name"));

  if (strlen(settings.getChar("bluetooth.name")) == 0)
  {
    Serial.printf("BT NAME: NULL\n", settings.getChar("bluetooth.name"));
    response = settings.setChar("bluetooth.name", strcat(productName, idDevice));
    settings.writeSettings(configFile);
    Serial.println("**RESTART**");
    ESP.restart();
  }
  else
    strcpy(bluetooth_name, settings.getChar("bluetooth.name"));

  strcpy(wifissidbuff, settings.getChar("wifi.ssid"));
  strcpy(wifipassbuff, settings.getChar("wifi.pass"));
  kwhtimestatbuff = settings.getInt("kwh.timeStatus");
  kwhtimenomibuff = settings.getInt("kwh.timeNominal");
  kwhfreqbuff = settings.getInt("kwh.freq");
  kwhstatusbeepbuff = settings.getInt("kwh.statusbeep");
  kwhalarmbuff = settings.getLong("kwh.alarm");

  Serial.printf("WIFI SSID: %s\n", wifissidbuff);
  Serial.printf("WIFI PASS: %s\n", wifipassbuff);
  Serial.printf("KWH TIME STATUS: %d\n", kwhtimestatbuff);
  Serial.printf("KWH TIME NOMINAL: %d\n", kwhtimenomibuff);
  Serial.printf("KWH FREQ: %d\n", kwhfreqbuff);
  Serial.printf("KWH STATUS BEEP: %d\n", kwhstatusbeepbuff);
  Serial.printf("KWH ALARM: %d\n", kwhalarmbuff);

  Serial.println("**SETTING INIT COMPLETE**");
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

void CheckPingGateway()
{
  int cntLoss;
  int pingReturn;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Ping.ping(customServer /* WiFi.localIP() */))
    {
      Serial.println("PING OK");
      cntLoss = 0;
    }
    else
    {
      counterPing++;
      Serial.println(counterPing);
      Serial.println("PING LOSS");
    }
  }
  // return cntLoss;
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
  if (counterPing >= 3)
  {
    MqttCustomPublish(topicRSPSignal, "1");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 1"));
    counterPing = 0;
  }
  else if (counterPing == 2)
  {
    MqttCustomPublish(topicRSPSignal, "2");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 2"));
    counterPing = 0;
  }
  else if (counterPing <= 1)
  {
    MqttCustomPublish(topicRSPSignal, "3");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 3"));
    counterPing = 0;
  }
}

void CheckPingBackground(void *parameter)
{
  pingTime = Alarm.timerRepeat(10, CheckPingServer);
  Alarm.alarmOnce(10, CheckPingSend);
  pingTimeSend = Alarm.timerRepeat(30, CheckPingSend);
  for (;;)
  {
    Alarm.delay(1000);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void dummySignalSend()
{

  MqttCustomPublish(topicRSPSignal, "3");
  // counterPing = 0;
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
  // long kwhalarm;
  char respBTBuffer[100];

  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("BT Client Connected");
    KoneksiStage = BT_INIT;
  }

  if (event == ESP_SPP_DATA_IND_EVT and KoneksiStage == BT_INIT)
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
        Serial.printf("DATA BT: ssid=%s pass=%s kwhstat=%s kwhnomi=%s kwhfreq=%s kwhstatusbeep=%s\n",
                      sPtr[0], sPtr[1], sPtr[2], sPtr[3], sPtr[4], sPtr[5]);
        kwhstat = atoi(sPtr[2]);
        kwhnomi = atoi(sPtr[3]);
        kwhfreq = atoi(sPtr[6]);
        kwhstatusbeep = atoi(sPtr[5]);
        // kwhalarm = atoi(sPtr[5]) * 60000;
        // }
        // Serial.printf("DATA READ BT: ssid=%s pass=%s kwhstat=%d kwhnomi=%d kwhfreq=%d kwhstatusbeep=%d\n",
        //               ssidBuffer, passBuffer, kwhstat, kwhnomi, kwhfreq, kwhstatusbeep);

        if (strcmp(wifissidbuff, ssidBuffer) == 0 and strcmp(wifipassbuff, passBuffer) == 0)
        {
          Serial.println("SAVE WIFI SAME --> NOT RESTART");
          SettingsSaveAll(ssidBuffer, passBuffer, kwhstat, kwhnomi, kwhfreq, kwhalarmbuff, kwhstatusbeep);
          SettingsInit();
        }

        else
        {
          Serial.println("SAVE WIFI CHANGE --> RESTART");
          SettingsSaveAll(ssidBuffer, passBuffer, kwhstat, kwhnomi, kwhfreq, kwhalarmbuff, kwhstatusbeep);
          SettingsInit();
          // WifiInit();
          ESP.restart();
        }
      }
    }
    else if (btReceive.length() < 5 and strcmp(btReceive.c_str(), "get") == 0)
    {
      sprintf(respBTBuffer, "ssid=%s pass=%s kwhstat=%d kwhnomi=%d kwhfreq=%d kwhstatusbeep=%d",
              wifissidbuff, wifipassbuff, kwhtimestatbuff, kwhtimenomibuff, kwhfreqbuff, kwhstatusbeepbuff);
      Serial.print("SEND EXISTING DATA -> ");
      Serial.println(respBTBuffer);
      SerialBT.println(respBTBuffer);
      SerialBT.flush();
      // free(respBTBuffer);
    }
    // if (N = 2)
    // {
    //   strcpy(ssidBuffer, sPtr[0]);
    //   strcpy(passBuffer, sPtr[1]);

    //   Serial.printf("DATA READ BT: ssid=%s pass=%s\n",
    //                 ssidBuffer, passBuffer);

    //   if (strcmp(wifissidbuff, ssidBuffer) == 0 and strcmp(wifipassbuff, passBuffer) == 0)
    //   {
    //     Serial.println("SAVE WIFI SAME --> NOT RESTART");
    //     SettingsSaveAll(ssidBuffer, passBuffer, kwhtimestatbuff, kwhtimenomibuff, kwhfreqbuff, kwhalarmbuff);
    //     SettingsInit();
    //   }

    //   else
    //   {
    //     Serial.println("SAVE WIFI CHANGE --> RESTART");
    //     SettingsSaveAll(ssidBuffer, passBuffer, kwhtimestatbuff, kwhtimenomibuff, kwhfreqbuff, kwhalarmbuff);
    //     SettingsInit();
    //     // WifiInit();
    //     ESP.restart();
    //   }
    // }
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

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case SYSTEM_EVENT_STA_CONNECTED:
    log_i("Connected to access point");
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    log_i("Disconnected from WiFi access point");
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    log_i("WiFi client disconnected");
    break;
  default:
    break;
  }
}

void WifiInit()
{
  Serial.println("**WIFI INIT START**");
  wifi_connect_cb();
  wifiReconRunner.init();
  wifiReconRunner.addTask(wifi_connect_task);
  wifiReconRunner.addTask(wifi_watchdog_task);
  WiFi.mode(wifi_mode_t::WIFI_STA);
  WiFi.onEvent(_wifi_event);
  wifi_connect_task.enableDelayed(5000);
  Serial.println("**WIFI INIT COMPLETE**");

  // #ifdef USEOTA
  //   OTASERVER.on("/", []()
  //                { OTASERVER.send(200, "text/plain", "Hi! I am ESP8266."); });

  //   ElegantOTA.begin(&OTASERVER); // Start ElegantOTA
  //   OTASERVER.begin();
  // #endif
}

void MqttCustomPublish(char *path, String msg);

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

  if (!postClient.connect(SERVER.c_str(), PORT))
  {
    return ("connection failed");
  }

  MqttPublishStatus("SENDING NOTIF SALDO");
  Serial.println("SENDING NOTIF SALDO");

  postClient.print(headerTxt);

  // WifiEspClient.stop();
  return String("UPLOADED");
  ;
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

String sendNotifSaldoImage(uint8_t *data_pic, size_t size_pic)
{
  String getAll;
  String getBody;
  WiFiClient postClient;

  Serial.println("Connecting to server: " + SERVER);

  if (postClient.connect(SERVER.c_str(), serverPort))
  {
    String serverRes;
    String bodyNotif = body_NotifLowSaldoImage();
    WiFiClient postClient;
    String bodyEnds = String("--") + BOUNDARY + String("--\r\n");
    size_t allLen = bodyNotif.length() + size_pic + bodyEnds.length();
    String headerTxt = headerNotifSaldoImage(idDevice, allLen);

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
  //   return String("UPLOADED");
  // }
  // else
  //   return String("NOT CONNECT");
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
    return;
  }
  else
    Serial.printf("CameraInit()...Succesfull\n", err);

  sensor_t *s = esp_camera_sensor_get();
  ov5640.start(s);
  if (s->id.PID == OV5640_PID)
  {
    s->set_vflip(s, 1);
    s->set_brightness(s, -2);
  }

  if (ov5640.focusInit() == 0)
  {
    Serial.println("OV5640_Focus_Init Successful!");
  }

  if (ov5640.autoFocusMode() == 0)
  {
    Serial.println("OV5640_Auto_Focus Successful!");
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
  loudness = calculateLoudness(energy, aweighting, OCTAVES, 1.0);
  SoundPeak = (int)floor(fft.MajorPeak());
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
  sprintf(topicRSPSignal, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "signal");
  sprintf(topicRSPStatus, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "debug");
  sprintf(topicRSPSoundStatus, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "sound/status");
  sprintf(topicRSPSoundPeak, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "sound/peak");
  sprintf(topicRSPSoundCnt, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "sound/cnt");
  sprintf(topicREQAlarm, "%s/%s/%s/%s", mqttChannel, "request", idDevice, "alarm");

  sprintf(topicREQtimeStatus, "%s/%s/%s/%s", mqttChannel, "request", idDevice, "delayStatus");
  sprintf(topicREQtimeNomi, "%s/%s/%s/%s", mqttChannel, "request", idDevice, "delayNominal");
  sprintf(topicREQstatusSound, "%s/%s/%s/%s", mqttChannel, "request", idDevice, "statusSound");
  sprintf(topicREQfreq, "%s/%s/%s/%s", mqttChannel, "request", idDevice, "freqPenempatan");

  // Serial.printf("%s\n", topicREQToken);
  // Serial.printf("%s\n", topicREQView);
  // Serial.printf("%s\n", topicRSPSignal);
  // Serial.printf("%s\n", topicRSPStatus);
}

/* *********************************** MQTT ************************* */
void MqttReconnect()
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "TokenOnline_" + String(idDevice);
    if (mqtt.connect(clientId.c_str()))
    {
      Serial.println("connected");
      MqttPublishStatus("CONNECTED MQTT");
      dummySignalSend();
      mqtt.subscribe(topicREQAlarm);
      mqtt.subscribe(topicREQtimeStatus);
      mqtt.subscribe(topicREQtimeNomi);
      mqtt.subscribe(topicREQstatusSound);
      mqtt.subscribe(topicREQfreq);
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

void CaptureAndSendNotif()
{
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

  // bool detectedShortItronFast2 = detectFrequency(&itronShortFast2, 6, SoundPeak, 186, 190, true);
  bool detectedShortItronFast = detectFrequency(&itronShortFast, 6, SoundPeak, kwhfreqbuff, 190, true);

  // Serial.printf("%d %d %d %d %d\n", SoundPeak, detectedShortItronFast, detectedShortItronFast2, detectShortCnt);
  if (detectedShortItronFast and (detectShortCnt > 20 /*25 and detectShortCnt < 35)*/))
  {
    // MqttCustomPublish(topicRSPSoundStatus, "DETECTED SHORT BEEP");
    // Serial.println("DETECTED SHORT BEEP");
    cntDetected = 0;
    detectShortCnt = 0;
    return true;
  }
  else
  {

    if (detectedShortItronFast and loudness > 43) // and !detectLong)
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
  bool detectedShortGagal = detectFrequency(&sounditronGagalKantor, 3, SoundPeak, 169, 192, true);
  // bool detectedShortItronFast = detectFrequency(&itronShortFast, 6, SoundPeak, 168, 199, true);

  return detectedShortGagal;
  // }
}

bool DetectionLowToken()
{
  // if (SoundDetectShortItron())
  // {
  //   timer.start();
  //   counter++;
  //   Serial.printf("COUNTER SHORT DETECT : %d\n", counter);
  //   // MqttCustomPublish(topicRSPSoundStatus, "COUNTER SOUND " + String(counter));
  // }
  // else if (timer.time_over())
  // {
  //   // Serial.print("TIMEOUT...");
  //   // Serial.println("RESET");
  //   counter = 0;
  // }

  // if (/* timer.time_over()  and */ counter > 7)
  // {
  //   MqttCustomPublish(topicRSPSoundStatus, "SEND NOTIFICATION SOUND");
  //   // sendNotifSaldo(idDevice);
  //   // Serial.println("DETECK LOW TOKEN");
  //   Serial.println("SEND NOTIFICATION");
  //   counter = 0;
  // }
  // return true;

  if (SoundDetectShortItron())
  {
    timer.start(2500);
    prevAlarmStatus = true;
    counter++;
    Serial.printf("COUNTER SHORT DETECT : %d\n", counter);
    MqttCustomPublish(topicRSPSoundPeak, String(SoundPeak));
    MqttCustomPublish(topicRSPSoundCnt, String(counter));
    // MqttCustomPublish(topicRSPSoundStatus, "COUNTER SOUND " + String(counter));
  }
  else if (timer.time_over())
  {
    // Serial.print("TIMEOUT...");
    // Serial.println("RESET");
    AlarmStatus = false;
    prevAlarmStatus = false;
    counter = 0;
  }
  if (counter > 3)
  {
    AlarmStatus = true;
  }

  if (TimeKwhAlarm.time_over())
  {
    // MqttCustomPublish("KWHALARM", "SEND NOTIF");
    timeAlarm = true;
  }

  if ((AlarmStatus and timeAlarm) or (prevAlarmStatus and timeAlarm))
  {
    MqttCustomPublish(topicRSPSoundStatus, "SEND NOTIFICATION SOUND");
    // sendNotifSaldo(idDevice);
    CaptureAndSendNotif();
    Serial.println("SEND NOTIFICATION");
    counter = 0;
    AlarmStatus = false;
    prevAlarmStatus = false;
    timeAlarm = false;
    TimeKwhAlarm.start(kwhalarmbuff);
  }
  return false;
}

// int SoundDetectValidasi()
// {
//   int returnValue;
//   long startTimer = millis();
//   while (millis() < startTimer + TIMEOUT_SUARA)
//   {
//     SoundLooping();
//     bool detectedGagal = detectFrequency(&soundValidasi, 2, SoundPeak, freqPeak, 196, true);

//     MqttCustomPublish(topicRSPSoundPeak, String(SoundPeak));
//     MqttCustomPublish(topicRSPSoundCnt, String(cntDetected));
//     if (detectedGagal)
//     {
//       cntDetected++;
//     }
//     else
//     {
//       soundValidasiCnt = cntDetected;
//       // Serial.println(detectCntItronGagal);
//       cntDetected = 0;
//       // Serial.println(detectCntItronGagal);
//       // detectCntItronGagal = 0;
//       // Serial.println("DETECK INVALID");
//       // break;
//     }
//     // returnValue = soundValidasiCnt;
//   }
//   // cntDetected = 0;
//   // soundValidasi = cntDetected;
//   // cntDetected = 0;
//   return cntDetected;
// }

int SoundDetectValidasi()
{
  int validasiCnt;
  SoundLooping();
  bool detectedGagal = detectFrequency(&soundValidasi, 2, SoundPeak, kwhfreqbuff, 196, true);

  MqttCustomPublish(topicRSPSoundPeak, String(SoundPeak));
  MqttCustomPublish(topicRSPSoundCnt, String(validasiCnt));
  if (detectedGagal and loudness > 40)
  {
    validasiCnt++;
  }
  else
  {
    soundValidasiCnt = validasiCnt;
    // Serial.println(detectCntItronGagal);
    // cntDetected = 0;
    // detectCntItronGagal = 0;
    // Serial.println("DETECK INVALID");
    // break;
  }

  // Serial.println("Timeout");
  return validasiCnt;
}

String OutputSoundValidasi(int val)
{
  String out;
  if (kwhstatusbeepbuff == 1)
  {
    if (val > 25)
    {
      Serial.println("DETECK GAGAL");
      MqttCustomPublish(topicRSPSoundStatus, "VALIDASI SUARA GAGAL");
      out = "failed";
    }

    else if (val != 0 && val < 25)
    {
      Serial.println("DETECK BENAR");
      MqttCustomPublish(topicRSPSoundStatus, "VALIDASI SUARA BENAR");
      out = "success";
    }
    else
    {
      Serial.println("DETECK INVALID");
      MqttCustomPublish(topicRSPSoundStatus, "VALIDASI SUARA INVALID");
      out = "invalid";
    }
  }
  else
  {
    Serial.println("DETECK INVALID");
    MqttCustomPublish(topicRSPSoundStatus, "VALIDASI SUARA INVALID");
    out = "invalid";
  }
  return out;
}

void GetKwhProcess()
{
  if (received_msg and strcmp(received_topic, topicREQView) == 0 /*and strcmp(received_payload, "view") == 0*/)
  {
    status = Status::GET_KWH;
    MqttPublishStatus("REQUESTING BALANCE");
    Serial.println("GET_KWH");
#ifdef USE_SOLENOID
    MechanicBackSpace();
#endif
    Serial.println(sendPhotoViewKWH(idDevice));
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

    // long tOut = millis() + TIMEOUT;
    // while (tOut > millis())
    // {
    //   Serial.print(".");
    //   delay(100);
    //   if (postClient.available())
    //   {
    //     serverRes = postClient.readStringUntil('\r');
    //     return (serverRes);
    //   }
    //   else
    //   {
    //     serverRes = "NOT RESPONSE";
    //     return (serverRes);
    //   }
    // }
    // // postClient.stop();
    // return serverRes;

    return String("UPLOADED");
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

  // delay(20);
  // long tOut = millis() + TIMEOUT;
  // while (tOut > millis())
  // {
  //   Serial.print(".");
  //   delay(100);
  //   if (postClient.available())
  //   {
  //     serverRes = postClient.readStringUntil('\r');
  //     return (serverRes);
  //   }
  //   else
  //   {
  //     serverRes = "NOT RESPONSE";
  //     return (serverRes);
  //   }
  // }
  // // postClient.stop();
  // return serverRes;

  return String("UPLOADED");
}

String sendImageStatusToken(String status, char *sn, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  WiFiClient postClient;
  String bodySatus = body_StatusToken();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySatus.length() + size_pic + bodyEnd.length();
  String headerTxt = headerStatusToken(status, sn, allLen);

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

  // delay(20);
  // long tOut = millis() + TIMEOUT;
  // while (tOut > millis())
  // {
  //   Serial.print(".");
  //   delay(100);
  //   if (postClient.available())
  //   {
  //     serverRes = postClient.readStringUntil('\r');
  //     return (serverRes);
  //   }
  //   else
  //   {
  //     serverRes = "NOT RESPONSE";
  //     return (serverRes);
  //   }
  // }
  // // postClient.stop();
  // return serverRes;

  return String("UPLOADED");
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

void ReceiveAlarm(/*char *token*/)
{
  if (received_msg and (strcmp(received_topic, topicREQAlarm) == 0))
  {
    int buffTimeAlarm;

    buffTimeAlarm = atoi(received_payload) * 60000;
    if (kwhalarmbuff == buffTimeAlarm)
      Serial.print("SAME ALARM SETTING");
    else
    {
      Serial.print("RECEIVE NEW DEVICE SETTING");
      Serial.println(buffTimeAlarm);
      kwhalarmbuff = buffTimeAlarm;
      Serial.print("VALID MILLIS: ");
      Serial.println(kwhalarmbuff);
      SettingsSaveMqttSetting(kwhtimestatbuff, kwhtimenomibuff, kwhfreqbuff, kwhalarmbuff, kwhstatusbeepbuff);

      TimeKwhAlarm.time_over();
      TimeKwhAlarm.start(kwhalarmbuff);
    }
    received_msg = false;
  }
}

void ReceiveTimeStatus(/*char *token*/)
{
  if (received_msg and (strcmp(received_topic, topicREQtimeStatus) == 0))
  {
    int buffTimeStatus;
    buffTimeStatus = atoi(received_payload);
    if (kwhtimestatbuff == buffTimeStatus)
      Serial.print("SAME TIME STATUS SETTING");
    else
    {
      Serial.print("RECEIVE NEW TIME STATUS");
      Serial.println(buffTimeStatus);
      kwhtimestatbuff = buffTimeStatus;
      SettingsSaveMqttSetting(kwhtimestatbuff, kwhtimenomibuff, kwhfreqbuff, kwhalarmbuff, kwhstatusbeepbuff);
    }
    received_msg = false;
  }
}

void ReceiveTimeNominal(/*char *token*/)
{
  if (received_msg and (strcmp(received_topic, topicREQtimeNomi) == 0))
  {
    int buffTimeNomi;
    buffTimeNomi = atoi(received_payload);
    if (kwhtimenomibuff == buffTimeNomi)
      Serial.print("SAME TIME NOMI SETTING");
    else
    {
      Serial.print("RECEIVE NEW TIME NOMINAL");
      Serial.println(buffTimeNomi);
      kwhtimenomibuff = buffTimeNomi;
      SettingsSaveMqttSetting(kwhtimestatbuff, kwhtimenomibuff, kwhfreqbuff, kwhalarmbuff, kwhstatusbeepbuff);
    }
    received_msg = false;
  }
}

void ReceiveStatusSound(/*char *token*/)
{
  if (received_msg and (strcmp(received_topic, topicREQstatusSound) == 0))
  {
    int buffStatusSound;
    buffStatusSound = atoi(received_payload);
    if (kwhstatusbeepbuff == buffStatusSound)
      Serial.print("SAME STATUS BEEP SETTING");
    else
    {
      Serial.print("RECEIVE NEW STATUS SOUND");
      Serial.println(buffStatusSound);
      kwhstatusbeepbuff = buffStatusSound;
      SettingsSaveMqttSetting(kwhtimestatbuff, kwhtimenomibuff, kwhfreqbuff, kwhalarmbuff, kwhstatusbeepbuff);
    }
    received_msg = false;
  }
}

void ReceiveFreq(/*char *token*/)
{
  if (received_msg and (strcmp(received_topic, topicREQfreq) == 0))
  {
    int buffFreq;
    buffFreq = atoi(received_payload);
    if (kwhfreqbuff == buffFreq)
      Serial.print("SAME FREKUENSI SETTING");
    else
    {
      Serial.print("RECEIVE NEW FREKUENSI");
      Serial.println(buffFreq);
      kwhfreqbuff = buffFreq;
      SettingsSaveMqttSetting(kwhtimestatbuff, kwhtimenomibuff, kwhfreqbuff, kwhalarmbuff, kwhstatusbeepbuff);
    }
    received_msg = false;
  }
}

// void TokenProcess(/*char *token*/)
// {
//   if (received_msg and (strcmp(received_topic, topicREQToken) == 0) and received_length == 20)
//   {
//     status = Status::PROCCESS_TOKEN;
//     MqttPublishStatus("EXECUTING TOKEN");
//     for (int i = 0; i < received_length; i++)
//     {

//       char chars = received_payload[i];
//       Serial.printf("%d ", i);
//       // MqttPublishStatus("SELENOID " + String(chars) + " ON");
//       switch (chars)
//       {
//       case '0':

//         MqttPublishStatus("SELENOID " + String(chars) + " ON");

//         // MechanicTyping(10);
//         delay(delaySolenoid);
//         break;

//       case '1':

//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(0);
//         delay(delaySolenoid);
//         break;

//       case '2':

//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(1);
//         delay(delaySolenoid);
//         break;
//       case '3':

//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(2);
//         delay(delaySolenoid);
//         break;
//       case '4':

//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(3);
//         delay(delaySolenoid);
//         break;
//       case '5':

//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(4);
//         delay(delaySolenoid);
//         break;
//       case '6':

//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(5);
//         delay(delaySolenoid);
//         break;
//       case '7':
//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(6);
//         delay(delaySolenoid);
//         break;
//       case '8':
//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(7);
//         delay(delaySolenoid);
//         break;
//       case '9':
//         MqttPublishStatus("SELENOID " + String(chars) + " ON");
//         // MechanicTyping(8);
//         delay(delaySolenoid);
//         break;
//       default:
//         break;
//       }
//     }
//     // MechanicEnter();
//     delay(delaySolenoid);
//     MqttPublishStatus("SELENOID ENTER ON");
//     delay(delayStatus);

//     camera_fb_t *fb = NULL;
//     fb = esp_camera_fb_get();
//     esp_camera_fb_return(fb); // dispose the buffered image
//     fb = NULL;                // reset to capture errors
//     fb = esp_camera_fb_get(); // get fresh image
//     // OutputSoundValidasi(SoundDetectValidasi());

//     String res;
//     if (!fb)
//     {
//       Serial.println("Camera capture failed");
//       // delay(1000);
//       ESP.restart();
//     }
//     else
//     {
//       res = sendImageStatusToken(idDevice, fb->buf, fb->len);
//       Serial.println(res);
//       esp_camera_fb_return(fb);
//     }

//     delay(delayNominal);
//     fb = NULL;
//     fb = esp_camera_fb_get();
//     esp_camera_fb_return(fb); // dispose the buffered image
//     fb = NULL;                // reset to capture errors
//     fb = esp_camera_fb_get(); // get fresh image
//     if (!fb)
//     {
//       Serial.println("Camera capture failed");
//       // delay(1000);
//       ESP.restart();
//     }
//     else
//     {
//       res = sendImageNominal(fb->buf, fb->len);
//       Serial.println(res);
//       esp_camera_fb_return(fb);
//     }

//     delay(delaySaldo);
//     fb = NULL;
//     fb = esp_camera_fb_get();
//     esp_camera_fb_return(fb); // dispose the buffered image
//     fb = NULL;                // reset to capture errors
//     fb = esp_camera_fb_get(); // get fresh image
//     if (!fb)
//     {
//       Serial.println("Camera capture failed");
//       delay(1000);
//       ESP.restart();
//     }
//     else
//     {
//       res = sendImageSaldo(received_payload, fb->buf, fb->len);
//       Serial.println(res);
//       esp_camera_fb_return(fb);
//     }

//     Serial.println("TOKEN PROCESS END");
//     received_msg = false;
//     status = Status::IDLE;
//   }
// }

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
#ifdef USE_SOLENOID
        MechanicTyping(10);
#endif
        delay(delaySolenoid);
        break;

      case '1':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
#ifdef USE_SOLENOID
        MechanicTyping(0);
#endif
        delay(delaySolenoid);
        break;

      case '2':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
#ifdef USE_SOLENOID
        MechanicTyping(1);
#endif
        delay(delaySolenoid);
        break;
      case '3':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
#ifdef USE_SOLENOID
        MechanicTyping(2);
#endif
        delay(delaySolenoid);
        break;
      case '4':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
#ifdef USE_SOLENOID
        MechanicTyping(3);
#endif
        delay(delaySolenoid);
        break;
      case '5':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
#ifdef USE_SOLENOID
        MechanicTyping(4);
#endif
        delay(delaySolenoid);
        break;
      case '6':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
#ifdef USE_SOLENOID
        MechanicTyping(5);
#endif
        delay(delaySolenoid);
        break;
      case '7':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
#ifdef USE_SOLENOID
        MechanicTyping(6);
#endif
        delay(delaySolenoid);
        break;
      case '8':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
#ifdef USE_SOLENOID
        MechanicTyping(7);
#endif
        delay(delaySolenoid);
        break;
      case '9':
        MqttPublishStatus("SELENOID " + String(chars) + " ON");
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
    MqttPublishStatus("SELENOID ENTER ON");
    Serial.println("ENTER");
    // delay(delayStatus);
    // validasiStartTimer = millis();
    afterSolenoid = millis();
    Serial.printf("afterSolenoid =%d\n", afterSolenoid);
    received_msg = false;
    TokenStateStatus = true;
  }

  if (status == Status::PROCCESS_TOKEN and millis() < afterSolenoid + kwhtimestatbuff)
  {
    camera_fb_t *fbs;
    Serial.printf("millis() =%d RecordValidasiSound()\n", millis());
    if (kwhstatusbeepbuff = 1)
      soundValidasiCnt = SoundDetectValidasi();
    if (millis() > afterSolenoid + ((kwhtimestatbuff / 2) + 100) and
        millis() < afterSolenoid + kwhtimestatbuff and TokenStateStatus)
    {

      String outputvalid = OutputSoundValidasi(soundValidasiCnt);
      Serial.printf("millis() =%d CaptureStatus()\n", millis());
      String res;
      fbs = esp_camera_fb_get();
      esp_camera_fb_return(fbs); // dispose the buffered image
      fbs = NULL;                // reset to capture errors
      fbs = esp_camera_fb_get(); // get fresh image
      if (!fbs)
      {
        Serial.println("Camera capture failed");
        delay(1000);
        ESP.restart();
      }
      else
      {
        res = sendImageStatusToken(outputvalid, idDevice, fbs->buf, fbs->len);
        Serial.println(res);
        esp_camera_fb_return(fbs);
      }
      TokenStateStatus = false;
      TokenStateNominal = true;
    }
  }

  /* if (status == Status::PROCCESS_TOKEN and
      millis() > afterSolenoid + (kwhtimestatbuff - 200) and
      millis() < afterSolenoid + kwhtimestatbuff and TokenStateStatus)
  {
    String outputvalid = OutputSoundValidasi(soundValidasiCnt);
    Serial.printf("millis() =%d CaptureStatus()\n", millis());
    if (TokenStateStatus)
    {
      Serial.println("CAPTURING STATUS");
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
        res = sendImageStatusToken(outputvalid, idDevice, fb->buf, fb->len);
        Serial.println(res);
        esp_camera_fb_return(fb);
      }
      TokenStateStatus = false;
      TokenStateNominal = true;
    }
    TokenStateStatus = false;
    TokenStateNominal = true;
  }
  else if (status == Status::PROCCESS_TOKEN and
           millis() > afterSolenoid + (kwhtimestatbuff - 200) and TokenStateStatus)
  {
    String outputvalid = OutputSoundValidasi(soundValidasiCnt);
    if (TokenStateStatus)
    {
      Serial.println("CAPTURING STATUS");
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
        res = sendImageStatusToken(outputvalid, idDevice, fb->buf, fb->len);
        Serial.println(res);
        esp_camera_fb_return(fb);
      }
      TokenStateStatus = false;
      TokenStateNominal = true;
    }
    TokenStateStatus = false;
    TokenStateNominal = true;
  } */

  if (status == Status::PROCCESS_TOKEN and
      millis() < (afterSolenoid + kwhtimestatbuff) + kwhtimenomibuff and
      millis() > (afterSolenoid + kwhtimestatbuff) + (kwhtimenomibuff - 200) and
      TokenStateNominal)
  {
    Serial.printf("millis() =%d CaptureNominal()\n", millis());
    if (TokenStateNominal)
    {
      Serial.println("CAPTURING NOMINAL");
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
        res = sendImageNominal(fb->buf, fb->len);
        Serial.println(res);
        esp_camera_fb_return(fb);
      }
      TokenStateNominal = false;
      TokenStateSaldo = true;
    }
    TokenStateNominal = false;
    TokenStateSaldo = true;
  }
  else if (status == Status::PROCCESS_TOKEN and
           millis() > (afterSolenoid + kwhtimestatbuff) + (kwhtimenomibuff - 200) and
           TokenStateNominal)
  {
    if (TokenStateNominal)
    {
      Serial.println("CAPTURING NOMINAL");
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
        res = sendImageNominal(fb->buf, fb->len);
        Serial.println(res);
        esp_camera_fb_return(fb);
      }
      TokenStateNominal = false;
      TokenStateSaldo = true;
    }
    TokenStateNominal = false;
    TokenStateSaldo = true;
  }

  if (status == Status::PROCCESS_TOKEN and
      millis() > (afterSolenoid + kwhtimestatbuff) + (kwhtimenomibuff + 2000) and TokenStateSaldo)
  {
    Serial.printf("millis() =%d CaptureSaldo()\n", millis());
    if (TokenStateSaldo)
    {
      Serial.println("CAPTURING SALDO");
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
        res = sendImageSaldo(received_payload, fb->buf, fb->len);
        Serial.println(res);
        esp_camera_fb_return(fb);
      }
      TokenStateSaldo = false;
      Serial.println("TOKEN PROCESS END");
      received_msg = false;
      status = Status::IDLE;
    }
    TokenStateSaldo = false;
    received_msg = false;
    status = Status::IDLE;
  }

  if (status == Status::PROCCESS_TOKEN and
      millis() > (afterSolenoid + kwhtimestatbuff) + (kwhtimenomibuff + 10000))
  {
    TokenStateStatus = false;
    TokenStateNominal = false;
    TokenStateSaldo = false;
    received_msg = false;
    status = Status::IDLE;
  }
}

void kirimAlarm()
{
  if (TimeKwhAlarm.time_over())
  {
    MqttCustomPublish("KWHALARM", "SEND NOTIF");
    CaptureAndSendNotif();
    Serial.printf("START ALARM: %d\n", kwhalarmbuff);
    TimeKwhAlarm.start(kwhalarmbuff);
  }
}

void connectToWiFi()
{
  log_i("connect to wifi");
  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.disconnect();
    WiFi.begin(wifissidbuff, wifipassbuff);
    log_i(" waiting on wifi connection");
    vTaskDelay(4000);
  }
  log_i("Connected to WiFi");
  WiFi.onEvent(WiFiEvent);
}

void WifiKeepAlive(void *pvParameters)
{
  for (;;)
  {
    if (!(WiFi.status() == WL_CONNECTED))
    {
      connectToWiFi();
      log_i("WiFi status %s", String(WiFi.status()));
      if (!(WiFi.status() == WL_CONNECTED))
      {
        connectToWiFi();
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);

  SettingsInit();
  KoneksiInit();
  WifiInit();
  CameraInit();
  delay(1000);
  SoundInit();
#ifdef USE_SOLENOID
  MechanicInit();
#endif

  mqtt.setServer(mqttServer, 1883);
  mqtt.setCallback(callback);
  MqttTopicInit();
  // timergagal.prepare(3000);
  // timer.prepare(5000);

  Alarm.timerOnce(10, dummySignalSend);
  // Alarm.timerRepeat(10, CheckPingGateway);
  pingTimeSend = Alarm.timerRepeat(30, dummySignalSend /* CheckPingSend */);
  pinMode(33, OUTPUT);
  digitalWrite(33, LOW);
  status = Status::IDLE;
}

void loop()
{
  wifiReconRunner.execute();
  if (WiFi.status() == WL_CONNECTED /*and wifiReconRunner.execute()*/)
  {
    if (!mqtt.connected() /*and status == Status::IDLE*/)
    {
      MqttReconnect();
    }
    if (status == Status::IDLE)
    {
      DetectionLowToken();
      // kirimAlarm();
    }
    mqtt.loop();
    ReceiveAlarm();
    ReceiveFreq();
    ReceiveStatusSound();
    ReceiveTimeNominal();
    ReceiveTimeStatus();
    TokenProcess();
    GetKwhProcess();
  }
  Alarm.delay(0);
  // }
}