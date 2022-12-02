#include <montok.h>

Adafruit_MCP23017 mcp;
HTTPClient httpClient;
WiFiClient WifiEspClient;
PubSubClient Mqtt(WifiEspClient);
Preferences preferences;
BluetoothSerial SerialBT;

const char *remote_host = "www.google.com";
const int ledPin = 12;
const int modeAddr = 0;
const int wifiAddr = 10;
String idDevice = "20221122";

String receivedData;

String wifiName;
String wifiPassword;
const char *post_url = "http://203.194.112.238:8081/montok/log"; // Location where images are POSTED
bool internet_connected = false;

bool wifiConnected = false;

int cnt_wifi_timeout = 0;
int modeIdx;

String network_string;
String connected_string;
String client_wifi_ssid;
String client_wifi_password;
enum wifi_setup_stages wifi_stage = NONE;
bool bluetooth_disconnect = false;

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

  CameraInit();
  SoundInit();
  MechanicInit();
  KoneksiInit();

  MqttConnect();

  // wifiStartTask();
  // bleTask();
  MqttPublish("203214223123144242");
  Mqtt.subscribe("monTok");
}

void loop()
{
  Mqtt.loop();
  // if (bluetooth_disconnect)
  // {
  //   disconnect_bluetooth();
  // }

  switch (wifi_stage)
  {
  case SCAN_START:
    SerialBT.println("Scanning Wi-Fi networks");
    Serial.println("Scanning Wi-Fi networks");
    scan_wifi_networks();
    SerialBT.println("Please enter the number for your Wi-Fi");
    wifi_stage = SCAN_COMPLETE;
    break;

  case SSID_ENTERED:
    SerialBT.println("Please enter your Wi-Fi password");
    Serial.println("Please enter your Wi-Fi password");
    wifi_stage = WAIT_PASS;
    break;

  case PASS_ENTERED:
    SerialBT.println("Please wait for Wi-Fi connection...");
    Serial.println("Please wait for Wi_Fi connection...");
    wifi_stage = WAIT_CONNECT;
    preferences.putString("pref_ssid", client_wifi_ssid);
    preferences.putString("pref_pass", client_wifi_password);
    if (init_wifi())
    { // Connected to WiFi
      connected_string = "ESP32 IP: ";
      connected_string = connected_string + WiFi.localIP().toString();
      SerialBT.println(connected_string);
      Serial.println(connected_string);
      // bluetooth_disconnect = true;
    }
    else
    { // try again
      wifi_stage = LOGIN_FAILED;
    }
    break;

  case LOGIN_FAILED:
    SerialBT.println("Wi-Fi connection failed");
    Serial.println("Wi-Fi connection failed");
    delay(2000);
    wifi_stage = SCAN_START;
    break;
  }
}