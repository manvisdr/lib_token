#include <Arduino.h>
// ---------------- WIFI -------------------//
#include <WiFi.h>
#include <ESP32Ping.h>
#include "WiFiClientSecure.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <BluetoothSerial.h>
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

//----------------- PIN ------------------//
#define PIN_MCP_SCL 15
#define PIN_MCP_SDA 14
#define PIN_SOUND_SCK 13 // BCK // CLOCK
#define PIN_SOUND_WS 2   // WS
#define PIN_SOUND_SD 12  // DATA IN

//----------------- PIN ------------------//

extern WiFiClient WifiEspClient;
extern PubSubClient Mqtt;
extern Adafruit_MCP23017 mcp;
extern Preferences preferences;
extern BluetoothSerial SerialBT;

extern String network_string;
extern String connected_string;
extern bool bluetooth_disconnect;

extern String idDevice;

enum wifi_setup_stages
{
  NONE,
  SCAN_START,
  SCAN_COMPLETE,
  SSID_ENTERED,
  WAIT_PASS,
  PASS_ENTERED,
  WAIT_CONNECT,
  LOGIN_FAILED
};

enum BT_client_state
{
  SCAN,
};

extern enum wifi_setup_stages wifi_stage;
extern String client_wifi_ssid;
extern String client_wifi_password;