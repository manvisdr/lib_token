#include <Arduino.h>
// ---------------- WIFI -------------------//
#include <WiFi.h>
#include <ESP32Ping.h>
#include "WiFiClientSecure.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <BluetoothSerial.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <Koneksi.h>
// ---------------- WIFI -------------------//

// ------------- SERVER-DATA --------------//
#include <PubSubClient.h>
#include <SendData.h>
// ------------- SERVER-DATA --------------//

// ---------------- CAMERA -------------------//
#include "esp_camera.h"
#include "EEPROM.h"
#define EEPROM_SIZE 128
#include "Camera.h"
#include "ESP32_OV5640_AF.h"
// ---------------- CAMERA -------------------//

// ---------------- SOUND -------------------//
#include <driver/i2s.h>
#include <math.h>
#include <arduinoFFT.h>
#include <Sound.h>
// ---------------- SOUND -------------------//

// ---------------- IO -------------------//
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Mechanic.h>
// ---------------- IO -------------------//

// ---------------- PERIPHERAL -----------//
#include <FS.h>
#include <LittleFS.h>
#include <SettingsManager.h>
#include <Settings.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
// ---------------- PERIPHERAL -----------//
#include <Proccess.h>
#include <timeout.h>

//----------------- PIN ------------------//
#define PIN_MCP_SCL 15
#define PIN_MCP_SDA 14
#define PIN_SOUND_SCK 13 // BCK // CLOCK
#define PIN_SOUND_WS 2   // WS
#define PIN_SOUND_SD 12  // DATA IN

//----------------- PIN ------------------//
extern WebServer OTASERVER;
extern WiFiClient WifiEspClient;
extern PubSubClient mqtt;
extern Adafruit_MCP23017 mcp;
extern Preferences preferences;
extern BluetoothSerial SerialBT;
extern OV5640 ov5640;
extern SettingsManager settings;
extern AlarmId pingTime;
extern AlarmId pingTimeSend;
extern Timeout timerSolenoid;

extern String network_string;
extern String connected_string;
extern bool bluetooth_disconnect;

enum ENUM_KONEKSI
{
  NONE,
  BT_INIT,
  WIFI_INIT,
  WIFI_OK,
  WIFI_FAILED
};

enum class Status : uint8_t
{
  Error,
  Ok,
  ConnectedMqtt,
  CapturingImage,
  RetreiveToken,
  ExecutingToken,
  SendingImageToken,
  RetreiveCheckBalance,
  SendingImageBalance,
  SendingNotifLowBalance,
};

extern enum ENUM_KONEKSI KoneksiStage;
extern Status DebugStatus;

extern String client_wifi_ssid;
extern String client_wifi_password;

extern char topicREQToken[50];
extern char topicRSPToken[50];
extern char topicREQView[50];
extern char topicRSPView[50];
extern char topicRSPSignal[50];
extern char topicRSPStatus[50];
extern char received_topic[128];
extern char received_payload[128];
extern unsigned int received_length;
extern bool received_msg;
extern char idDevice[10];
extern char bluetooth_name[20];
extern char wifissidbuff[20];
extern char wifipassbuff[20];

extern IPAddress mqttServer;