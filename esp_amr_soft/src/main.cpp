#include <Arduino.h>
#include <SPI.h>
#include <lorawan.h>
#include <EthernetSPI2.h>
#include <PubSubClient.h>
#include <AutoConnect.h>
#include <FS.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#define GET_CHIPID() (ESP.getChipId())
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#define GET_CHIPID() ((uint16_t)(ESP.getEfuseMac() >> 32))
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#ifdef AUTOCONNECT_USE_SPIFFS
FS &FlashFS = SPIFFS;
#else
#include <LittleFS.h>
FS &FlashFS = LittleFS;
#endif
#elif defined(ARDUINO_ARCH_ESP32)
#include <SPIFFS.h>
fs::SPIFFSFS &FlashFS = SPIFFS;
#endif

#if defined(ARDUINO_ARCH_ESP8266)
typedef ESP8266WebServer WiFiWebServer;
#elif defined(ARDUINO_ARCH_ESP32)
typedef WebServer WiFiWebServer;
#endif

// ASCII FRAMING HEX
#define ACK 0x06  // ACKNOWLEDGE
#define CAN 0x18  // ACKNOWLEDGE
#define ENQ 0x05  // ASCII 0x05 == ENQUIRY
#define STX 0x02  // START OF TEXT
#define ETX 0x03  // END OF TEXT
#define EOT 0x04  // END OF TRANSMISSION
#define LF 0x0A   // LINEFEED
#define NUL 0x00  // NULL termination character
#define XOFF 0x13 // XOFF Pause transmission
#define XON 0x11  // XON Resume transmission
#define DLE 0x10  // DATA LINE ESCAPE
#define CR 0x0D   // CARRIAGE RETURN \r
#define LF 0x0A   // LINE FEED \n

// HANDLE SERIAl
#define MSG_INTERVAL 5000            // send an enquiry every 5-sec
#define CHAR_TIMEOUT 2000            // expect chars within a second (?)
#define BUFF_SIZE 50                 // buffer size (characters)
#define MAX_RX_CHARS (BUFF_SIZE - 2) // number of bytes we allow (allows for NULL term + 1 byte margin)
#define LED_BLIP_TIME 300            // mS time LED is blipped each time an ENQ is sent

// STATUS SERIAL NAMA
#define ST_STX 0
#define ST_ETX 1
#define ST_ACK 2
#define ST_CAN 3
#define ST_CON 4
#define ST_LOGIN 5
#define ST_READ 6
#define ST_RX_READ 7
#define ST_LOGOUT 8
#define ST_MSG_TIMEOUT 9

// STATUS REGISTER KWH
#define ST_REG_SERIAL 0
#define ST_REG_KWH_A 1
#define ST_REG_KWH_B 2
#define ST_REG_KWH_T 3
#define ST_REG_END 4

// DEF TIPE REGISTER
#define CHAR_REGREAD 0x52
#define CHAR_LOGREAD 0x4C

// DEF PIN
#define RXD2 16                  // PIN RX2
#define TXD2 17                  // PIN TX2
#define csEth 15                 // PIN CS ETHERNET
#define csLora 5                 // PIN CS LORA
const byte pinLED = LED_BUILTIN; // visual indication of life
const sRFM_pins RFM_pins = {
    .CS = 5,
    .RST = 26,
    .DIO0 = 25,
    .DIO1 = 32,
}; // DEF PIN LORA

#define MODE_LORA 0  // LORA
#define MODE_ETH_D 1 // LAN DHCP
#define MODE_ETH_S 2 // LAN STATIC

// Define Link Page Web Server
#define PARAM_FILE "/param.json"
#define AUX_SETTING_URI "/mqtt_setting"
#define AUX_SAVE_URI "/mqtt_save"

#define PARAM_FILE2 "/lan.json"
#define AUX_SETTING_URI2 "/lan_setting"
#define AUX_SAVE_URI2 "/lan_save"

#define PARAM_FILE3 "/profile.json"
#define AUX_SETTING_URI3 "/profile_setting"
#define AUX_SAVE_URI3 "/profile_save"

#define PARAM_FILE4 "/socket.json"
#define AUX_SETTING_URI4 "/socket_setting"
#define AUX_SAVE_URI4 "/socket_save"

#define PARAM_FILE5 "/lora.json"
#define AUX_SETTING_URI5 "/lora_setting"
#define AUX_SAVE_URI5 "/lora_save"

#define AUX_RESET_URI "/amr_reset"
#define AUX_REBOOT_URI "/amr_reboot"

// MQTT CONF
AutoConnect portal;
AutoConnectConfig config;
WiFiClient wifiClient;
EthernetClient ethClient;
PubSubClient mqttClientWifi(wifiClient);
PubSubClient mqttClientEth(ethClient);
#define MQTT_USER_ID "anyone"

//  Var CONFIG DEFAULT
String serverName;
String customerID;
String kwhType;
String serverCheck;
String channelId = "monKWH/AMR";
String Port;
String userKey;
String apiKey;
String apid;
String ipAddres;
String ipTCP;
String local_portTCP;
String remote_portTCP;
String ipCheck;
String Gateway;
String DNS;
String netMask;
String dataProfile;
String devAddrLora;
String nwkSKeyLora;
String appSKeyLora;
bool checkDHCP;
bool enableWifi;
bool enableLAN;
bool enableLora;
bool enableConverter;
bool factoryReset;
bool rebootDevice;
unsigned int interval = 5000;
long previousMillis = 0;

static const char AUX_mqtt_setting[] PROGMEM = R"raw(
[
  {
    "title": "MQTT",
    "uri": "/mqtt_setting",
    "menu": true,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>MQTT Settings</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "caption",
        "type": "ACText",
        "value": "MQTT Broker Setting",
        "style": "color:#4682b4;"
      },
       {
        "name": "mqttserver",
        "type": "ACInput",
        "value": "",
        "label": "Server",
        "pattern": "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$",
        "placeholder": "MQTT broker server"
      },
      {
        "name": "port",
        "type": "ACInput",
        "label": "Port"
      },
      {
        "name": "userkey",
        "type": "ACInput",
        "label": "Username"
      },
      {
        "name": "apikey",
        "type": "ACInput",
        "label": "Password",
        "apply": "password"
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save",
        "uri": "/mqtt_save"
      },
      {
        "name": "discard",
        "type": "ACSubmit",
        "value": "Discard",
        "uri": "/"
      }
    ]
  },
  {
    "title": "Settings",
    "uri": "/mqtt_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parameters",
        "type": "ACText"
      }
    ]
  }
]
)raw";

static const char AUX_lan_setting[] PROGMEM = R"raw(
[
  {
    "title": "LAN",
    "uri": "/lan_setting",
    "menu": true,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>LAN Setting</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
       {
      "name": "checkdhcp",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Enable DHCP",
      "labelposition": "infront",
      "checked": true
    },
     {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr><br>"
      },
      {
        "name": "caption",
        "type": "ACText",
        "value": "Static IP Address<br>",
        "style": "color:#2f4f4f;"
      },
      {
        "name": "ipaddres",
        "type": "ACInput",
        "value": "",
        "label": "IP addres",
        "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
        "placeholder": "IP address"
      },
      {
        "name": "gateway",
        "type": "ACInput",
        "value": "",
        "label": "Gateway",
        "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"
      },
      {
        "name": "netmask",
        "type": "ACInput",
        "value": "",
        "label": "Netmask",
        "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
        "placeholder": ""
      },
       {
        "name": "dns",
        "type": "ACInput",
        "value": "",
        "label": "DNS",
        "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
        "placeholder": ""
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save",
        "uri": "/lan_save"
      }
    ]
  },
  {
    "title": "LAN",
    "uri": "/lan_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parameters",
        "type": "ACText"
      }
    ]
  }
]
)raw";

static const char AUX_lora_setting[] PROGMEM = R"raw(
[
  {
    "title": "Lora",
    "uri": "/lora_setting",
    "menu": true,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>Lora Parameters</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "devAddrLora",
        "type": "ACInput",
        "value": "",
        "label": "Dev ADDR"
      },
      {
        "name": "appSKeyLora",
        "type": "ACInput",
        "value": "",
        "label": "App Session Key:"
      },
      {
        "name": "nwkSkeyLora",
        "type": "ACInput",
        "value": "",
        "label": "Network Session Key:"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save Parameters",
        "uri": "/lora_save"
      }
    ]
  },
  {
    "title": "Lora",
    "uri": "/lora_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parametersLora",
        "type": "ACText"
      }
    ]
  }
]
)raw";

static const char AUX_socket_setting[] PROGMEM = R"raw(
[
  {
    "title": "Socket",
    "uri": "/socket_setting",
    "menu": true,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>Socket Setting</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "ipTCP",
        "type": "ACInput",
        "value": "",
        "label": "IP Address",
        "pattern": "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$",
        "placeholder": "IP address"
      },
      {
        "name": "local_portTCP",
        "type": "ACInput",
        "value": "",
        "label": "Local Port"
      },
        {
        "name": "remote_portTCP",
        "type": "ACInput",
        "value": "",
        "label": "Remote Port"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save Configuration",
        "uri": "/socket_save"
      }
    ]
  },
  {
    "title": "Socket",
    "uri": "/socket_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parametersSocket",
        "type": "ACText"
      }
    ]
  }
]
)raw";

static const char AUX_profile_setting[] PROGMEM = R"raw(
[
  {
    "title": "Profile",
    "uri": "/profile_setting",
    "menu": true,
    "element": [
      {
        "name": "style",
        "type": "ACStyle",
        "value": "label+input,label+select{position:sticky;left:120px;width:230px!important;box-sizing:border-box;}"
      },
       {
        "name": "header",
        "type": "ACText",
        "value": "<h2>Profile Configuration</h2>",
        "style": "text-align:left;color:#2f4f4f;padding:10px;"
      },
       {
        "name": "customerID",
        "type": "ACInput",
        "value": "",
        "label": "Customer ID"
      },
       {
        "name": "kwhType",
        "type": "ACInput",
        "value": "",
        "label": "KWH Type"
      },
       {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "caption2",
        "type": "ACText",
        "value": "Network Enable Option<br>",
        "style": "color:#2f4f4f;"
      },
      {
      "name": "enableWifi",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Enable Wifi",
      "labelposition": "inbehind",
      "checked": false
    },
    {
      "name": "enableLAN",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Enable LAN",
      "labelposition": "inbehind",
      "checked": false
    },
    {
      "name": "enableLora",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Enable Lora",
      "labelposition": "inbehind",
      "checked": false
    },
     {
        "name": "newline2",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "caption3",
        "type": "ACText",
        "value": "Converter Mode<br>",
        "style": "color:#2f4f4f;"
      },
      {
      "name": "enableConverter",
      "type": "ACCheckbox",
      "value": "check",
      "label": "Enable Converter",
      "labelposition": "inbehind",
      "checked": false
    },
       {
        "name": "space",
        "type": "ACText",
        "value": "<br><br>"
      },
      {
        "name": "newline3",
        "type": "ACElement",
        "value": "<hr>"
      },
       {
        "name": "caption4",
        "type": "ACText",
        "value": "System Reset<br>",
        "style": "color:#2f4f4f;"
      },
      {
        "name": "reset",
        "type": "ACSubmit",
        "value": "Reset Device",
        "style": "color:#ffd966;",
        "uri": "/amr_reset"
        
      },
      {
        "name": "reboot",
        "type": "ACSubmit",
        "style": "color:#ffd966;",
        "value": "Reboot Device",
        "uri": "/amr_reboot"
        
      },
      {
        "name": "newline4",
        "type": "ACElement",
        "value": "<hr><br>"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save Configuration",
        "uri": "/profile_save"
      }
    ]
  },
  {
    "title": "profile",
    "uri": "/profile_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parameterProfile",
        "type": "ACText"
      }
    ]
  }
]
)raw";

// DEF LORA CONF

// DEF ETHERNET CONF
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ipStatic;
EthernetServer server(23);

// VAR MODE KIRIM
bool gotAMessage = false;
bool modeLora = false;
bool modeIp = false;

// VAR KWH REGISTER
unsigned char logonReg[15] = {0x4C, 0x45, 0x44, 0x4D, 0x49, 0x2C, 0x49, 0x4D, 0x44, 0x45, 0x49, 0x4D, 0x44, 0x45, 0x00};
unsigned char logoffReg[1] = {0x58};
byte regSerNum[2] = {0xF0, 0x02};
byte regKWHA[2] = {0x1E, 0x01};
byte regKWHB[2] = {0x1E, 0x02};
byte regKWHTOT[2] = {0x1E, 0x00};

// VAR HANDLE SERIAL
char rxTextFrame[BUFF_SIZE];
bool bLED;
unsigned long timeLED;

// PROTOTYPE FUNCTION
void ReceiveStateMachine(void);
bool GetSerialChar(char *pDest, unsigned long *pTimer);
bool CheckTimeout(unsigned long *timeNow, unsigned long *timeStart);

bool mqttConnectWifi()
{
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
  char clientId[9];
  uint8_t retry = 3;
  while (!mqttClientWifi.connected())
  {
    if (serverName.length() <= 0)
      break;

    mqttClientWifi.setServer(serverName.c_str(), 1883);
    // Serial.println(String("Attempting MQTT broker:") + serverName);

    for (uint8_t i = 0; i < 8; i++)
    {
      clientId[i] = alphanum[random(62)];
    }
    clientId[8] = '\0';
    // Setting mqtt gaes
    if (mqttClientWifi.connect(clientId, userKey.c_str(), apiKey.c_str()))
    {
      // Serial.println("Established:" + String(clientId));
      return true;
    }
    else
    {
      if (!--retry)
        break;
      delay(3000);
    }
  }
  return false;
}

bool mqttConnectEth()
{
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
  char clientId[9];
  uint8_t retry = 3;
  while (!mqttClientEth.connected())
  {
    if (serverName.length() <= 0)
      break;

    mqttClientEth.setServer(serverName.c_str(), 1883);
    // Serial.println(String("Attempting MQTT broker:") + serverName);

    for (uint8_t i = 0; i < 8; i++)
    {
      clientId[i] = alphanum[random(62)];
    }
    clientId[8] = '\0';
    // Setting mqtt gaes
    if (mqttClientEth.connect(clientId, userKey.c_str(), apiKey.c_str()))
    {
      // Serial.println("Established:" + String(clientId));
      return true;
    }
    else
    {
      if (!--retry)
        break;
      delay(3000);
    }
  }
  return false;
}

void mqttPublish(String msg)
{
  String path = channelId + customerID;
  if (enableWifi)
  {
    mqttClientWifi.publish(path.c_str(), msg.c_str());
  }
  if (enableLAN)
  {
    mqttClientEth.publish(path.c_str(), msg.c_str());
  }
}

void (*amr_rebootDevice)(void) = 0;

void getParams(AutoConnectAux &aux)
{
  serverName = aux["mqttserver"].value;
  serverName.trim();
  Port = aux["port"].value;
  Port.trim();
  userKey = aux["userkey"].value;
  userKey.trim();
  apiKey = aux["apikey"].value;
  apiKey.trim();
}

void getParamsLan(AutoConnectAux &aux)
{
  checkDHCP = aux["checkdhcp"].as<AutoConnectCheckbox>().checked;
  ipAddres = aux["ipaddres"].value;
  ipAddres.trim();
  Gateway = aux["gateway"].value;
  Gateway.trim();
  netMask = aux["netmask"].value;
  netMask.trim();
  DNS = aux["dns"].value;
  DNS.trim();
}

void getParamsLora(AutoConnectAux &aux)
{
  devAddrLora = aux["devAddrLora"].value;
  devAddrLora.trim();
  nwkSKeyLora = aux["nwkSKeyLora"].value;
  nwkSKeyLora.trim();
  appSKeyLora = aux["appSKeyLora"].value;
  appSKeyLora.trim();
}

void getParamsSocket(AutoConnectAux &aux)
{
  ipTCP = aux["ipTCP"].value;
  ipTCP.trim();
  local_portTCP = aux["local_portTCP"].value;
  local_portTCP.trim();
  remote_portTCP = aux["remote_portTCP"].value;
  remote_portTCP.trim();
}

void getParamsProfile(AutoConnectAux &aux)
{
  customerID = aux["customerID"].value;
  customerID.trim();
  kwhType = aux["kwhType"].value;
  kwhType.trim();
  enableWifi = aux["enableWifi"].as<AutoConnectCheckbox>().checked;
  enableLAN = aux["enableLAN"].as<AutoConnectCheckbox>().checked;
  enableLora = aux["enableLora"].as<AutoConnectCheckbox>().checked;
  enableConverter = aux["enableConverter"].as<AutoConnectCheckbox>().checked;
}

String loadParams(AutoConnectAux &aux, PageArgument &args)
{
  (void)(args);
  File param = FlashFS.open(PARAM_FILE, "r");
  if (param)
  {
    if (aux.loadElement(param))
    {
      getParams(aux);
      // Serial.println(PARAM_FILE " loaded");
    }
    else
      // Serial.println(PARAM_FILE " failed to load");
      param.close();
  }
  else
  {
    // Serial.println(PARAM_FILE " open failed");
  }
  return String("");
}

String loadParamsLan(AutoConnectAux &aux, PageArgument &args)
{
  (void)(args);
  File paramLan = FlashFS.open(PARAM_FILE2, "r");
  if (paramLan)
  {
    if (aux.loadElement(paramLan))
    {
      getParamsLan(aux);
      // Serial.println(PARAM_FILE " loaded");
    }
    else
      // Serial.println(PARAM_FILE " failed to load");
      paramLan.close();
  }
  else
  {
    // Serial.println(PARAM_FILE " open failed");
  }
  return String("");
}

String loadParamsLora(AutoConnectAux &aux, PageArgument &args)
{
  (void)(args);
  File paramLora = FlashFS.open(PARAM_FILE5, "r");
  if (paramLora)
  {
    if (aux.loadElement(paramLora))
    {
      getParamsLora(aux);
      // Serial.println(PARAM_FILE " loaded");
    }
    else
      // Serial.println(PARAM_FILE " failed to load");
      paramLora.close();
  }
  else
  {
    // Serial.println(PARAM_FILE " open failed");
  }
  return String("");
}

String loadParamsSocket(AutoConnectAux &aux, PageArgument &args)
{
  (void)(args);
  File paramSocket = FlashFS.open(PARAM_FILE4, "r");
  if (paramSocket)
  {
    if (aux.loadElement(paramSocket))
    {
      getParamsSocket(aux);
      // Serial.println(PARAM_FILE " loaded");
    }
    else
      // Serial.println(PARAM_FILE " failed to load");
      paramSocket.close();
  }
  else
  {
    // Serial.println(PARAM_FILE " open failed");
  }
  return String("");
}

String loadParamsProfile(AutoConnectAux &aux, PageArgument &args)
{
  (void)(args);
  File paramProfile = FlashFS.open(PARAM_FILE3, "r");
  if (paramProfile)
  {
    if (aux.loadElement(paramProfile))
    {
      getParamsProfile(aux);
      // Serial.println(PARAM_FILE " loaded");
    }
    else
      // Serial.println(PARAM_FILE " failed to load");
      paramProfile.close();
  }
  else
  {
    // Serial.println(PARAM_FILE " open failed");
  }
  return String("");
}

String saveParams(AutoConnectAux &aux, PageArgument &args)
{
  AutoConnectAux &mqtt_setting = *portal.aux(portal.where());
  getParams(mqtt_setting);
  // AutoConnectInput& mqttserver = mqtt_setting["mqttserver"].as<AutoConnectInput>();

  File param = FlashFS.open(PARAM_FILE, "w");
  mqtt_setting.saveElement(param, {"mqttserver", "port", "userkey", "apikey"});

  String sendConfig = "mqtt*" + String(serverName) + "*" + String(userKey) + "*" + String(apiKey) + "*" + String(Port) + "#";
  Serial.println(sendConfig);

  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText &echo = aux["parameters"].as<AutoConnectText>();
  echo.value = "Server: " + serverName + "<br>";
  echo.value += "User Key: " + userKey + "<br>";
  echo.value += "Port: " + Port + "<br>";

  return String("");
}

String saveParamsLan(AutoConnectAux &aux, PageArgument &args)
{
  AutoConnectAux &lan_setting = *portal.aux(portal.where());
  getParamsLan(lan_setting);

  File paramLan = FlashFS.open(PARAM_FILE2, "w");
  lan_setting.saveElement(paramLan, {"ipaddres", "gateway", "netmask", "dns", "checkdhcp"});
  int condDHCP;
  if (checkDHCP)
  {
    condDHCP = 0;
  }
  else
  {
    condDHCP = 1;
  }
  String sendConfig = "IP*" + String(ipAddres) + "*" + String(Gateway) + "*" + String(netMask) + "*" + String(DNS) + "*" + String(condDHCP) + "#";
  Serial.println(sendConfig);
  paramLan.close();
  AutoConnectText &echo = aux["parameters"].as<AutoConnectText>();
  echo.value += "IP Addres: " + ipAddres + "<br>";
  echo.value += "Gateway: " + Gateway + "<br>";
  echo.value += "Netmask: " + netMask + "<br>";
  echo.value += "DNS: " + DNS + "<br>";
  echo.value += "Enable DHCP: " + String(checkDHCP == true ? "true" : "false") + "<br>";
  return String("");
}

String saveParamsSocket(AutoConnectAux &aux, PageArgument &args)
{
  AutoConnectAux &socket_setting = *portal.aux(portal.where());
  getParamsSocket(socket_setting);

  File paramSocket = FlashFS.open(PARAM_FILE4, "w");
  socket_setting.saveElement(paramSocket, {"ipTCP", "local_portTCP", "remote_portTCP"});
  paramSocket.close();
  AutoConnectText &echo = aux["parametersSocket"].as<AutoConnectText>();
  echo.value += "TCP/IP Address: " + ipTCP + "<br>";
  echo.value += "Local Port: " + local_portTCP + "<br>";
  echo.value += "Remote Port: " + remote_portTCP + "<br>";
  return String("");
}

String saveParamsLora(AutoConnectAux &aux, PageArgument &args)
{
  AutoConnectAux &lora_setting = *portal.aux(portal.where());
  getParamsLora(lora_setting);

  File paramLora = FlashFS.open(PARAM_FILE5, "w");
  lora_setting.saveElement(paramLora, {"devAddrLora", "nwkSKeyLora", "appSKeyLora"});
  paramLora.close();
  AutoConnectText &echo = aux["parametersLora"].as<AutoConnectText>();
  echo.value += "Dev ADDR: " + devAddrLora + "<br>";
  echo.value += "APP Session Key: " + appSKeyLora + "<br>";
  echo.value += "Network Session Key: " + nwkSKeyLora + "<br>";
  return String("");
}

String saveParamsProfile(AutoConnectAux &aux, PageArgument &args)
{
  AutoConnectAux &profile_setting = *portal.aux(portal.where());
  getParamsProfile(profile_setting);
  File paramProfile = FlashFS.open(PARAM_FILE3, "w");
  profile_setting.saveElement(paramProfile, {"customerID", "kwhType", "enableWifi", "enableLAN", "enableLora", "enableConverter"});
  Serial.println(dataProfile);
  paramProfile.close();
  AutoConnectText &echo = aux["parameterProfile"].as<AutoConnectText>();
  echo.value += "Customer ID: " + customerID + "<br>";
  echo.value += "Type KWH: " + kwhType + "<br>";
  echo.value += "Enable Wifi: " + String(enableWifi == true ? "true" : "false") + "<br>";
  echo.value += "Enable LAN: " + String(enableLAN == true ? "true" : "false") + "<br>";
  echo.value += "Enable Lora: " + String(enableLora == true ? "true" : "false") + "<br>";
  echo.value += "Enable Converter: " + String(enableConverter == true ? "true" : "false") + "<br>";
  return String("");
}

void handleRoot()
{
  String content =
      "<html>"
      "<head>"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "</head>"
      "<body>"
      "<H1 style=\"display:block;text-align:center;font-family:serif;\">AMR Setting</H1>"
      "<p style=\"padding-top:5px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
                                                                                 "</body>"
                                                                                 "</html>";

  // content.replace("{{CHANNEL}}", channelId);
  WiFiWebServer &webServer = portal.host();
  webServer.send(200, "text/html", content);
}

void handleReboot()
{
  HTTPClient httpClient;
  Serial.println("Device Reboot");
  WiFiWebServer &webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String("/"));
  webServer.send(302, "text/plain", "");
  webServer.client().stop();
  amr_rebootDevice();
}

void handleReset()
{
  HTTPClient httpClient;
  Serial.println("Device Reset");
  AutoConnectAux &mqtt_setting = *portal.aux(AUX_SETTING_URI);
  getParams(mqtt_setting);

  mqtt_setting["mqttserver"].value = "207.148.122.42";
  mqtt_setting["port"].value = String(1883);
  mqtt_setting["userkey"].value = "mgidas";
  mqtt_setting["apikey"].value = String(1234509876);

  serverName = mqtt_setting["mqttserver"].value;
  serverName.trim();
  Port = mqtt_setting["port"].value;
  Port.trim();
  userKey = mqtt_setting["userkey"].value;
  userKey.trim();
  apiKey = mqtt_setting["apikey"].value;
  apiKey.trim();

  File param = FlashFS.open(PARAM_FILE, "w");
  if (param)
  {
    mqtt_setting.saveElement(param, {"mqttserver", "port", "userkey", "apikey"});
    param.close();
    Serial.println("Data Tersimpan");
  }
  else
  {
    Serial.println("open file error");
  }

  AutoConnectCredential credential;
  station_config_t config;
  uint8_t ent = credential.entries();

  credential.load((int8_t)0, &config);
  if (credential.del((const char *)&config.ssid[0]))
    Serial.printf("%s deleted.\n", (const char *)config.ssid);
  else
    Serial.printf("%s failed to delete.\n", (const char *)config.ssid);

  WiFi.disconnect();
  WiFiWebServer &webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String("/"));
  webServer.send(302, "text/plain", "");
  webServer.client().stop();
}

bool loraConf()
{
  // if (!lora.init())
  // {
  //   Serial.println("DEBUG: loraConf()");
  //   Serial.println("    LORA NOT DETECTED - WAIT 5 SEC");
  //   delay(5000);
  //   return false;
  // }
  lora.setDeviceClass(CLASS_C);
  lora.setDataRate(SF10BW125);
  lora.setChannel(MULTI);
  lora.setNwkSKey(nwkSKeyLora.c_str());
  lora.setAppSKey(appSKeyLora.c_str());
  lora.setDevAddr(devAddrLora.c_str());
  return true;
}

bool checkSendMode()
{
  if (enableLora)
  {
    Serial.println(F("DEBUG: MODE LORA"));
    loraConf();
  }
  if (enableLAN and checkDHCP)
  {
    if (Ethernet.begin(mac) == 0)
    {
      Serial.println(F("DEBUG: ETH: Failed DHCP"));
      if (Ethernet.hardwareStatus() == EthernetNoHardware)
      {
        Serial.println(F("DEBUG: ETH DHCP: ETH NOT FOUND:"));
        Serial.println(F("      Ethernet shield was not found"));
        enableLora = true;
      }
      else if (Ethernet.linkStatus() == LinkOFF)
      {
        Serial.println(F("DEBUG: ETH DHCP:"));
        Serial.println(F("      Ethernet cable is not connected..."));
        enableLora = true;
      }
    }
  }
  else if (enableLAN)
  {
    Ethernet.begin(mac, ipStatic.fromString(ipAddres));
    Serial.println(F("DEBUG: ETH STATIC: USE STATIC"));
    Serial.print(F("      DHCP IP : "));
    Serial.println(Ethernet.localIP());
  }
  server.begin();
  return true;
}

short gencrc_16(short i)
{
  short j;
  short k;
  short crc;
  k = i << 8;
  crc = 0;
  for (j = 0; j < 8; j++)
  {
    if ((crc ^ k) & 0x8000)
      crc = (crc << 1) ^ 0x1021;
    else
      crc <<= 1;
    k <<= 1;
  }
  return (crc);
}

unsigned short
CalculateCharacterCRC16(unsigned short crc, unsigned char c)
{
  return ((crc << 8) ^ gencrc_16((crc >> 8) ^ c));
}

// SEND TO KWH
void cmdlink_putch(unsigned char ch)
{
  Serial2.write(ch);
}

void send_byte(unsigned char d)
{
  switch (d)
  {
  case STX:
  case ETX:
  case DLE:
  case XON:
  case XOFF:
    cmdlink_putch(DLE);
    cmdlink_putch(d | 0x40);
    break;
  default:
    cmdlink_putch(d);
  }
}

void send_cmd(unsigned char *cmd, unsigned short len)
{
  unsigned short i;
  unsigned short crc;
  /*
   * Add the STX and start the CRC calc.
   */
  cmdlink_putch(STX);
  crc = CalculateCharacterCRC16(0, STX);
  /*
   * Send the data, computing CRC as we go.
   */
  for (i = 0; i < len; i++)
  {
    send_byte(*cmd);
    crc = CalculateCharacterCRC16(crc, *cmd++);
  }
  /*
   * Add the CRC
   */
  send_byte((unsigned char)(crc >> 8));
  send_byte((unsigned char)crc);
  /*
   * Add the ETX
   */
  cmdlink_putch(ETX);
}

bool cmd_trigMeter()
{
  cmdlink_putch(STX);
  cmdlink_putch(ETX);
  return true;
}

bool cmd_logonMeter()
{
  send_cmd(logonReg, sizeof(logonReg));
  return true;
}

bool cmd_logoffMeter()
{
  send_cmd(logoffReg, sizeof(logoffReg));
  return true;
}

void cmd_readRegister(const byte *reg)
{
  unsigned char readReg[3] = {0x52, reg[0], reg[1]};
  send_cmd(readReg, sizeof(readReg));
}

void ReceiveStateMachine(void)
{
  char
      ch;
  static byte
      rxPtr = 0;
  static byte
      regCnt = 0;
  static byte
      stateReg = ST_REG_SERIAL;
  static unsigned long
      timeMsg = 0,
      timeRX;
  unsigned long
      timeNow;
  static byte
      stateNext,
      stateRX = ST_CON;

  timeNow = millis();
  switch (stateRX)
  {
  case ST_CON:
    if ((timeNow - timeMsg) >= MSG_INTERVAL)
    {
      // turn on the LED for visual indication
      bLED = true;
      timeLED = timeNow;
      digitalWrite(pinLED, HIGH);
      cmd_trigMeter();
      timeRX = timeNow;   // character timeout
      stateRX = ST_LOGIN; // wait for the start of text

    } // if

    break;

  case ST_LOGIN:
    // skip over the header; we're looking for the STX character
    if (GetSerialChar(&ch, &timeRX) == true)
    {
      if (ch == ACK)
      {
        Serial.println(F("DEBUG: ST_LOGIN:"));
        Serial.println(F("    CONNECT ACK."));
        // got it; move to receive body of msg
        cmd_logonMeter();
        stateRX = ST_READ;

      } // if

    } // if
    else
    {
      // check for a timeout
      if (CheckTimeout(&timeNow, &timeRX))
      {
        Serial.println(F("DEBUG: ST_LOGIN:"));
        Serial.println(F("      TIMEOUT..."));
        stateRX = ST_MSG_TIMEOUT;
      }

    } // else

    break;

  case ST_READ:
    // skip over the header; we're looking for the STX character
    if (GetSerialChar(&ch, &timeRX) == true)
    {
      if (ch == ACK)
      {
        Serial.println(F("DEBUG: ST_READ:"));
        Serial.println(F("    LOGIN ACK."));
        // got it; move to receive body of msg
        switch (stateReg)
        {
        case ST_REG_SERIAL:
          Serial.println(F("      READ  - SERIAL NUMBER."));
          cmd_readRegister(regSerNum);
          stateReg = ST_REG_KWH_A;
          break;
        case ST_REG_KWH_A:
          Serial.println(F("      READ  - KWH A."));
          cmd_readRegister(regKWHA);
          stateReg = ST_REG_KWH_B;
          break;
        case ST_REG_KWH_B:
          Serial.println(F("      READ  - KWH B."));
          cmd_readRegister(regKWHB);
          stateReg = ST_REG_KWH_T;
          break;
        case ST_REG_KWH_T:
          Serial.println(F("      READ  - KWH TOTAL."));
          cmd_readRegister(regKWHTOT);
          stateReg = ST_REG_END;
          break;
        }
        stateRX = ST_RX_READ;

      } // if

    } // if
    else
    {
      // check for a timeout
      if (CheckTimeout(&timeNow, &timeRX))
      {
        Serial.println(F("DEBUG: ST_READ:"));
        Serial.println(F("      TIMEOUT..."));
        stateRX = ST_MSG_TIMEOUT;
      }

    } // else

    break;

  case ST_RX_READ:
    // receive characters into rxBuff until we see a LF
    if (GetSerialChar(&ch, &timeRX) == true)
    {
      // have we received the EOT token?
      if (ch == CAN)
      {
        Serial.println(F("DEBUG: ST_RX_READ:"));
        Serial.println(F("    CAN - CANCEL"));
        // got it; move to receive body of msg
        stateRX = ST_LOGOUT;
      } // if// if
      else if (ch == CHAR_REGREAD)
      {
        Serial.println(F("DEBUG: ST_RX_READ: GET DATA"));
        // got a LF; add a NULL termination...
        rxTextFrame[rxPtr] = NUL;

        //...then do something with the message
        Serial.print(F("      "));
        Serial.println(rxTextFrame[0]); // send to serial monitor
        //...

        // now reset the buffer pointer; keep RXing and processing messages at the LF delimiter
        // until we see the ETX token
        rxPtr = 0;

        if (stateReg == ST_REG_END)
        {
          Serial.println(F("DEBUG: ST_RX_READ: READ END"));
          Serial.println(F("      LOGOUT..."));
          cmd_logoffMeter();
          stateRX = ST_LOGOUT;
          stateReg = ST_REG_SERIAL;
        }
        else
          stateRX = ST_READ;

      } // else if
      else
      {
        // receive another character into the buffer
        rxTextFrame[rxPtr] = ch;

        // protect the buffer from overflow
        if (rxPtr < MAX_RX_CHARS)
          rxPtr++;

      } // else

    } // if
    else
    {
      // check for a timeout
      if (CheckTimeout(&timeNow, &timeRX))
      {
        Serial.println(F("DEBUG: ST_RX_READ:"));
        Serial.println(F("      TIMEOUT..."));
        stateRX = ST_MSG_TIMEOUT;
      }

    } // else

    break;

  case ST_LOGOUT:
    // receive characters into rxBuff until we see a LF
    if (GetSerialChar(&ch, &timeRX) == true)
    {
      // have we received the EOT token?
      if (ch == ACK)
      {
        Serial.println(F("DEBUG: ST_LOGOUT:"));
        Serial.println(F("    LOGOUT ACK"));
        // yes; reset ENQ timer and return to that state
        timeMsg = timeNow;
        stateRX = ST_CON;

      } // if

    } // if
    else
    {
      // check for a timeout
      if (CheckTimeout(&timeNow, &timeRX))
      {
        Serial.println(F("DEBUG: ST_LOGOUT:"));
        Serial.println(F("      TIMEOUT..."));
        stateRX = ST_MSG_TIMEOUT;
      }

    } // else

    break;

  case ST_MSG_TIMEOUT:
    // we timed out waiting for a character; display error
    Serial.println(F("DEBUG: ST_MSG_TIMEOUT:")); // send debug msg to serial monitor
    Serial.println(F("      Timeout waiting for character"));

    // probably not needed since we just timed out but flush the serial buffer anyway
    while (Serial2.available())
      ch = Serial2.read();

    // then reset back to ENQ state
    timeMsg = timeNow;
    stateRX = ST_CON;

    break;

  } // switch

} // ReceiveStateMachine

bool GetSerialChar(char *pDest, unsigned long *pTimer)
{
  if (Serial2.available())
  {
    // get the character
    *pDest = Serial2.read();

    // and reset the character timeout
    *pTimer = millis();

    return true;

  } // if
  else
    return false;

} // GetSerialChar

bool CheckTimeout(unsigned long *timeNow, unsigned long *timeStart)
{
  if (*timeNow - *timeStart >= CHAR_TIMEOUT)
    return true;
  else
    return false;

} // CheckTimeout

void ServiceLED(void)
{
  if (!bLED)
    return;

  if ((millis() - timeLED) >= LED_BLIP_TIME)
  {
    digitalWrite(pinLED, LOW);
    bLED = false;

  } // if

} // ServiceLED

bool mode_checked = false;

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(pinLED, OUTPUT);

  Ethernet.init(csEth);
  lora.init();

#if defined(ARDUINO_ARCH_ESP8266)
  FlashFS.begin();
#elif defined(ARDUINO_ARCH_ESP32)
  FlashFS.begin(true);
#endif

  config.ota = AC_OTA_BUILTIN;

  if (portal.load(FPSTR(AUX_mqtt_setting)))
  {
    AutoConnectAux &mqtt_setting = *portal.aux(AUX_SETTING_URI);
    PageArgument args;
    loadParams(mqtt_setting, args);
    config.homeUri = "/";
    portal.on(AUX_SETTING_URI, loadParams);
    portal.on(AUX_SAVE_URI, saveParams);
  }
  if (portal.load(FPSTR(AUX_lan_setting)))
  {
    AutoConnectAux &lan_setting = *portal.aux(AUX_SETTING_URI2);
    PageArgument args;
    loadParamsLan(lan_setting, args);
    config.homeUri = "/";
    portal.on(AUX_SETTING_URI2, loadParamsLan);
    portal.on(AUX_SAVE_URI2, saveParamsLan);
  }
  if (portal.load(FPSTR(AUX_lora_setting)))
  {
    AutoConnectAux &lora_setting = *portal.aux(AUX_SETTING_URI5);
    PageArgument args;
    loadParamsLora(lora_setting, args);
    config.homeUri = "/";
    portal.on(AUX_SETTING_URI5, loadParamsLora);
    portal.on(AUX_SAVE_URI5, saveParamsLora);
  }
  if (portal.load(FPSTR(AUX_socket_setting)))
  {
    AutoConnectAux &socket_setting = *portal.aux(AUX_SETTING_URI4);
    PageArgument args;
    loadParamsSocket(socket_setting, args);
    config.homeUri = "/";
    portal.on(AUX_SETTING_URI4, loadParamsSocket);
    portal.on(AUX_SAVE_URI4, saveParamsSocket);
  }
  if (portal.load(FPSTR(AUX_profile_setting)))
  {
    AutoConnectAux &profile_setting = *portal.aux(AUX_SETTING_URI3);
    PageArgument args;
    loadParamsProfile(profile_setting, args);
    config.homeUri = "/";
    portal.on(AUX_SETTING_URI3, loadParamsProfile);
    portal.on(AUX_SAVE_URI3, saveParamsProfile);
  }

  else
  { // Serial.println("load error");
  }
  config.apid = "MEL_AMR0001";
  config.hostName = "MEL_AMR0001";
  config.autoReset = false;
  config.autoReconnect = true;
  config.reconnectInterval = 1;
  config.portalTimeout = 5000;
  config.retainPortal = true;
  portal.config(config);
  portal.disableMenu(AC_MENUITEM_RESET | AC_MENUITEM_HOME);
  //  portal.load(FPSTR(PAGE_HELLO));

  // Serial.print("WiFi ");
  if (portal.begin())
  {
    //      Serial.print("IP:" + WiFi.localIP().toString());
    config.bootUri = AC_ONBOOTURI_HOME;
  }
  else
  {
  }

  WiFiWebServer &webServer = portal.host();
  webServer.on("/", handleRoot);
  webServer.on(AUX_RESET_URI, handleReset);
  webServer.on(AUX_REBOOT_URI, handleReboot);

} // setup

void loop()
{
  // ReceiveStateMachine(); // messaging with meter
  // ServiceLED();          // for LED timing
  if (!mode_checked)
    mode_checked = checkSendMode();

  if (WiFi.status() == WL_CONNECTED && enableWifi == true)
  {

    if (!mqttClientWifi.connected())
    {
      mqttConnectWifi();
    }

    if (millis() - previousMillis > interval)
    {
      // MGI Command Line Format
      String dataKirim = "*" + customerID + "*dataKWH1*dataKWH2*dataKWH3*totalKWH#";
      mqttPublish(dataKirim);
      //        mqttClient.loop();
      previousMillis = millis();
    }
  }
  if (enableLAN == true)
  {

    if (!mqttClientEth.connected())
    {
      mqttConnectEth();
    }

    if (millis() - previousMillis > interval)
    {
      // MGI Command Line Format
      String dataKirim = "*" + customerID + "*dataKWH1*dataKWH2*dataKWH3*totalKWH#";
      mqttPublish(dataKirim);
      //        mqttClient.loop();
      previousMillis = millis();
    }
  }
  else
  {
  }
  portal.handleClient();

} // loop
