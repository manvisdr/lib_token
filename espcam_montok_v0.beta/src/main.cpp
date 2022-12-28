#include <tokenonline.h>

WebServer OTASERVER(80);
SettingsManager settings;
Adafruit_MCP23017 mcp;
HTTPClient httpClient;
WiFiClient WifiEspClient;
PubSubClient mqtt(WifiEspClient);
Preferences preferences;
BluetoothSerial SerialBT;
OV5640 ov5640 = OV5640();
IPAddress mqttServer(203, 194, 112, 238);
AlarmId pingTime;
AlarmId pingTimeSend;
Timeout timerSolenoid;

char idDevice[10];
char bluetooth_name[20];
enum ENUM_KONEKSI KoneksiStage = NONE;
Status DebugStatus;

char topicREQToken[50];
char topicRSPToken[50];
char topicREQView[50];
char topicRSPView[50];
char topicRSPSignal[50];
char topicRSPStatus[50];
char wifissidbuff[20];
char wifipassbuff[20];

char received_topic[128];
char received_payload[128];
unsigned int received_length;
bool received_msg = false;

void setup()
{
  Serial.begin(115200);
  pinMode(33, OUTPUT);
  digitalWrite(33, HIGH);

  SettingsInit();
  KoneksiInit();

  WifiInit();
  CameraInit();
  SoundInit();
  ProccessInit();
  MqttInit();

  MechanicInit();

  digitalWrite(33, LOW);
  delay(300);
  digitalWrite(33, HIGH);
  delay(300);
  digitalWrite(33, LOW);

  // for (int a = 0; a < 20; a++)
  // {
  //   MechanicTyping(dummyToken[a]);
  // }
}

void loop()
{
  if (!mqtt.connected())
  {
    MqttReconnect();
  }
  mqtt.loop();

  TokenProcess();
  GetKwhProcess();
  DetecLowToken();

  OTASERVER.handleClient();
}