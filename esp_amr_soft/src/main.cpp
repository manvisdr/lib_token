#include <Arduino.h>
#include <SPI.h>
#include <lorawan.h>
#include <EthernetSPI2.h>
#include <EthernetUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <AutoConnect.h>
#include <FS.h>
#include <SafeString.h>

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
#define BUFF_SIZE 20                 // buffer size (characters)
#define MAX_RX_CHARS (BUFF_SIZE - 2) // number of bytes we allow (allows for NULL term + 1 byte margin)
#define LED_BLIP_TIME 300            // mS time LED is blipped each time an ENQ is sent
#define SERIALNUM_CHAR 10
#define MAX_KWH_CHAR 4
#define MAX_ACK_CHAR 4
#define MAX_KWH_OUT_CHAR 16
uint32_t const READ_DELAY = 1000; /* ms */

// STATUS SERIAL
#define ST_STX 0
#define ST_ETX 1
#define ST_ACK 2
#define ST_CAN 3
#define ST_CON 4
#define ST_LOGIN 5
#define ST_LOGIN_R 6
#define ST_READ 7
#define ST_RX_READ 8
#define ST_LOGOUT 9
#define ST_MSG_TIMEOUT 10

// STATUS REGISTER KWH
#define ST_REG_SERIAL 0
#define ST_REG_KWH_A 1
#define ST_REG_KWH_B 2
#define ST_REG_KWH_T 3
#define ST_REG_END 4

// DEF TIPE REGISTER
#define CHAR_REGREAD 0x52 // CODE EDMI BACA REGISTER R
#define CHAR_LOGREAD 0x4C // CODE EDMI LOGIN L
#define CHAR_LOGOUT 0x58

// DEF PIN
#define RXD2 16                  // PIN RX2
#define TXD2 17                  // PIN TX2
#define csEth 15                 // PIN CS ETHERNET
#define csEth2 4                 // PIN CS ETHERNET
#define csLora 5                 // PIN CS LORA
const byte pinLED = LED_BUILTIN; // INDICATOR LED SERIAL
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
EthernetClient mqttEthClient;
PubSubClient mqttClientEth(mqttEthClient);
#define MQTT_USER_ID "anyone"

//  Var CONFIG DEFAULT
String serverName;
String customerID;
String kwhID;
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
String kwhA;
String kwhB;
String kwhTOT;
bool checkDHCP;
bool enableWifi;
bool enableLAN;
bool enableLora;
bool enableConverter;
bool factoryReset;
bool rebootDevice;
unsigned int interval = 5000;
long previousMillis = 0;
bool saveIPAddress;
bool saveIPSerial;

// DEF LORA CONF

// DEF ETHERNET CONF
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ipStatic;
IPAddress ipGateway;
IPAddress ipNetmask;
IPAddress ipDNS;
IPAddress ipSerial;
int portEthSerial;
EthernetServer server(23);
EthernetClient serialModeEthClient;
EthernetUDP ntpUDP;
const long utcOffsetInSeconds = 25200; // GMT INDONESIA FOR NTP
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);

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

// VAR HASIL READ
char outSerialNumber[SERIALNUM_CHAR];
char outKWHA[MAX_KWH_OUT_CHAR];
char outKWHB[MAX_KWH_OUT_CHAR];
char outKWHTOT[MAX_KWH_OUT_CHAR];

// VAR HANDLE SERIAL
char rxTextFrame[BUFF_SIZE];
bool bLED;
unsigned long timeLED;
bool getKWHData = false;

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

// PROTOTYPE FUNCTION
void ReceiveStateMachine(void);
bool GetSerialChar(char *pDest, unsigned long *pTimer);
bool CheckTimeout(unsigned long *timeNow, unsigned long *timeStart);

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
    // //server.println(String("Attempting MQTT broker:") + serverName);

    for (uint8_t i = 0; i < 8; i++)
    {
      clientId[i] = alphanum[random(62)];
    }
    clientId[8] = '\0';
    // Setting mqtt gaes
    if (mqttClientEth.connect(clientId, userKey.c_str(), apiKey.c_str()))
    {
      // //server.println("Established:" + String(clientId));
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
      // //server.println(PARAM_FILE " loaded");
    }
    else
      // //server.println(PARAM_FILE " failed to load");
      param.close();
  }
  else
  {
    // //server.println(PARAM_FILE " open failed");
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
      // //server.println(PARAM_FILE " loaded");
    }
    else
      // //server.println(PARAM_FILE " failed to load");
      paramLan.close();
  }
  else
  {
    // //server.println(PARAM_FILE " open failed");
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
      // //server.println(PARAM_FILE " loaded");
    }
    else
      // //server.println(PARAM_FILE " failed to load");
      paramLora.close();
  }
  else
  {
    // //server.println(PARAM_FILE " open failed");
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
      // //server.println(PARAM_FILE " loaded");
    }
    else
      // //server.println(PARAM_FILE " failed to load");
      paramSocket.close();
  }
  else
  {
    // //server.println(PARAM_FILE " open failed");
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
      // //server.println(PARAM_FILE " loaded");
    }
    else
      // //server.println(PARAM_FILE " failed to load");
      paramProfile.close();
  }
  else
  {
    // //server.println(PARAM_FILE " open failed");
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
  // server.println(sendConfig);

  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText &echo = aux["parameters"].as<AutoConnectText>();
  echo.value = "Server: " + serverName + "<br>";
  echo.value += "User Key: " + userKey + "<br>";
  echo.value += "Port: " + Port + "<br>";

  return String("");
}

void handleReboot()
{
  HTTPClient httpClient;
  // server.println("Device Reboot");
  WiFiWebServer &webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String("/"));
  webServer.send(302, "text/plain", "");
  webServer.client().stop();
  amr_rebootDevice();
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
  // server.println(sendConfig);
  paramLan.close();
  AutoConnectText &echo = aux["parameters"].as<AutoConnectText>();
  echo.value += "IP Addres: " + ipAddres + "<br>";
  echo.value += "Gateway: " + Gateway + "<br>";
  echo.value += "Netmask: " + netMask + "<br>";
  echo.value += "DNS: " + DNS + "<br>";
  echo.value += "Enable DHCP: " + String(checkDHCP == true ? "true" : "false") + "<br>";
  delay(1000);
  handleReboot();
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
  // server.println(dataProfile);
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

void handleReset()
{
  HTTPClient httpClient;
  // server.println("Device Reset");
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
    // server.println("Data Tersimpan");
  }
  else
  {
    // server.println("open file error");
  }

  AutoConnectCredential credential;
  station_config_t config;
  uint8_t ent = credential.entries();

  credential.load((int8_t)0, &config);
  if (credential.del((const char *)&config.ssid[0]))
    server.printf("%s deleted.\n", (const char *)config.ssid);
  else
    server.printf("%s failed to delete.\n", (const char *)config.ssid);

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
  //   //server.println("DEBUG: loraConf()");
  //   //server.println("    LORA NOT DETECTED - WAIT 5 SEC");
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
    // server.println(F("DEBUG: MODE LORA"));
    loraConf();
  }
  if (enableLAN)
  {
    // server.print(F("DEBUG: enableLAN: DHCP: "));
    if (checkDHCP)
    {
      if (Ethernet.begin(mac) == 0)
      {
        // server.println(F("DEBUG: ETH: Failed DHCP"));
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
          // server.println(F("DEBUG: ETH DHCP: ETH NOT FOUND:"));
          // server.println(F("      Ethernet shield was not found"));
          enableLora = true;
        }
        else if (Ethernet.linkStatus() == LinkOFF)
        {
          // server.println(F("DEBUG: ETH DHCP:"));
          // server.println(F("      Ethernet cable is not connected..."));
          //  enableLora = true;
        }
      }
      // server.print(F("      DHCP IP : "));
      // server.println(Ethernet.localIP());
    }
    else if (enableConverter)
    {
      // saveIPAddress = ipStatic.fromString(ipAddres);
      saveIPSerial = ipSerial.fromString(ipAddres);
      if (serialModeEthClient.connect(ipSerial, portEthSerial))
      {
        // server.println("Server Serial Converter Connected");
      }
      else
      {
        // server.println("Server Serial Converter Failed");
      }
    }
    else if (!enableConverter)
    {
      saveIPAddress = ipStatic.fromString(ipAddres);
      saveIPAddress = ipGateway.fromString(Gateway);
      saveIPAddress = ipNetmask.fromString(netMask);
      saveIPAddress = ipDNS.fromString(DNS);
      Ethernet.begin(mac, ipStatic, ipDNS, ipGateway, ipNetmask);
      // server.println(F("DEBUG: ETH STATIC: USE STATIC"));
      // server.print(F("      STATIC IP : "));
      // server.println(Ethernet.localIP());
    }
    // NTP SET
    timeClient.begin();
    timeClient.update();
    timeClient.setUpdateInterval(5 * 60 * 1000); // update from NTP only every 5 minutes
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

float ConvertB32ToFloat(uint32_t b32)
{
  float result;
  int32_t shift;
  uint16_t bias;

  if (b32 == 0)
    return 0.0;
  // pull significand
  result = (b32 & 0x7fffff); // mask significand
  result /= (0x800000);      // convert back to float
  result += 1.0f;            // add one back
  // deal with the exponent
  bias = 0x7f;
  shift = ((b32 >> 23) & 0xff) - bias;
  while (shift > 0)
  {
    result *= 2.0;
    shift--;
  }
  while (shift < 0)
  {
    result /= 2.0;
    shift++;
  }
  // sign
  result *= (b32 >> 31) & 1 ? -1.0 : 1.0;
  return result;
}

enum class kwhStatus : uint8_t
{
  Ready,
  Busy,
  Ok,
  Timeout,
  Notlogin,
  ProtocolError,
  ChecksumError,
};

enum class kwhStep : uint8_t
{
  Ready,
  Started,
  SendConnect,
  Connected,
  SendLogin,
  LoginACK,
  SendRegister,
  ReadRegister,
  AfterData,
  SendLogout,
  LogoutACK,
};

enum class kwhRegisterStep : uint8_t
{
  SerialNumber,
  KWHA,
  KWHB,
  KWHTOTAL,
};

size_t errors_ = 0, checksum_errors_ = 0, successes_ = 0;
kwhStatus status_;
kwhStep step_;
kwhRegisterStep regStep_;

void kwh_ready()
{
  if (status_ != kwhStatus::Busy)
    status_ = kwhStatus::Ready;
}

void kwh_start_reading()
{
  /* Don't allow starting a read when one is already in progress */
  if (status_ == kwhStatus::Busy)
    return;

  status_ = kwhStatus::Busy;
  step_ = kwhStep::Started;
  regStep_ = kwhRegisterStep::KWHA;
}

void kwh_change_status(kwhStatus to)
{
  if (to == kwhStatus::ProtocolError)
    ++errors_;
  else if (to == kwhStatus::ChecksumError)
    ++checksum_errors_;
  else if (to == kwhStatus::Ok)
    ++successes_;

  status_ = to;
}

void kwh_SendConnect()
{
  cmd_trigMeter();
  Serial2.flush();
  delay(100);
  // server.println(F("kwh_SendConnect()"));
  step_ = kwhStep::Connected;
}

void kwh_sendLogin()
{
  // server.println(F("kwh_sendLogin()"));
  cmd_logonMeter();
  Serial2.flush();
  delay(100);
  // server.println(F("SEND LOGIN"));
  step_ = kwhStep::LoginACK;
}

void kwh_readACK()
{
  // server.print(F("kwh_readACK() -> "));
  char charACK[MAX_ACK_CHAR];
  char flushRX;
  if (Serial2.available() > 0)
  {
    flushRX = Serial2.read();
  }
  size_t len = Serial2.readBytesUntil(ETX, charACK, MAX_ACK_CHAR);
  // server.println(charACK);

  if (charACK[0] == ACK or charACK[1] == ACK)
  {
    if (step_ == kwhStep::Connected)
    {
      // server.println(F("ACK RECEIVED - kwhStep::Connected"));
      step_ = kwhStep::SendLogin;
    }
    else if (step_ == kwhStep::LoginACK)
    {
      // server.println(F("ACK RECEIVED - kwhStep::LoginACK"));
      step_ = kwhStep::SendRegister;
    }
    else if (step_ == kwhStep::AfterData)
    {
      // server.println(F("ACK RECEIVED - kwhStep::AfterData"));
      step_ = kwhStep::LogoutACK;
    }
  }
}

void kwh_parseData()
{
  // server.println(F("kwh_parseData()"));
  char charData[MAX_RX_CHARS];
  char buffData[MAX_KWH_CHAR];
  createSafeString(outParse, 8);
  char flushRX;
  char s[16];
  uint32_t value;

  // if (Serial2.available() > 0)
  // {
  //   // server.println(Serial2.read());

  size_t len = Serial2.readBytesUntil(ETX, charData, MAX_RX_CHARS);
  //   server.println(charData);
  // }
  server.println(len);
  if (len < 9)
  {
    if (Serial2.available() > 0)
    {
      flushRX = Serial2.read();
      server.println(flushRX);
    }
    size_t len2 = Serial2.readBytesUntil(ETX, charData, MAX_RX_CHARS);
    server.println(len2);
  }
  //   if (len2 < 9)
  // {
  //   if (Serial2.available() > 0)
  //   {
  //     flushRX = Serial2.read();
  //   }
  //   size_t len3 = Serial2.readBytesUntil(ETX, charData, MAX_RX_CHARS);
  //    server.println(len3);
  // }
  // }

  server.println(charData);

  switch (regStep_)
  {

  case kwhRegisterStep::KWHA:
    server.println(F("     DATA KWH_A "));
    for (size_t i = 0; i < MAX_KWH_CHAR; i++)
    {
      buffData[i] = charData[i + 4];
      outParse += "0123456789ABCDEF"[buffData[i] / 16];
      outParse += "0123456789ABCDEF"[buffData[i] % 16];
      server.print(F("     DATA ke "));
      server.print(i);
      server.print(F(" > "));
      server.print(charData[i + 4], HEX);
      server.print(F(" | BUFFDATA ="));
      server.println(buffData[i], HEX);
    }
    // server.print(F("     "));
    // server.println(buffData);
    // //server.print(convert((byte)buffData)); using union

    // server.println(outParse);
    value = strtoul(outParse.c_str(), NULL, 16);
    dtostrf(ConvertB32ToFloat(value), 1, 0, outKWHA);
    server.println(outKWHA);
    outParse.clear();

    break;

  case kwhRegisterStep::KWHB:
    server.println(F("     DATA KWH_B "));
    for (size_t i = 0; i < MAX_KWH_CHAR; i++)
    {
      buffData[i] = charData[i + 5];
      outParse += "0123456789ABCDEF"[buffData[i] / 16];
      outParse += "0123456789ABCDEF"[buffData[i] % 16];
      server.print(F("     DATA ke "));
      server.print(i);
      server.print(F(" > "));
      server.print(charData[i + 5], HEX);
      server.print(F(" | BUFFDATA ="));
      server.println(buffData[i], HEX);
    }
    // server.print(F("     "));
    // server.println(buffData);

    // server.println(outParse);
    value = strtoul(outParse.c_str(), NULL, 16);
    dtostrf(ConvertB32ToFloat(value), 1, 0, outKWHB);
    server.println(outKWHB);
    outParse.clear();

    break;

  case kwhRegisterStep::KWHTOTAL:
    server.println(F("     DATA KWH_T "));
    for (size_t i = 0; i < MAX_KWH_CHAR; i++)
    {
      buffData[i] = charData[i + 4];
      outParse += "0123456789ABCDEF"[buffData[i] / 16];
      outParse += "0123456789ABCDEF"[buffData[i] % 16];
      server.print(F("     DATA ke "));
      server.print(i);
      server.print(F(" > "));
      server.print(charData[i + 4], HEX);
      server.print(F(" | BUFFDATA ="));
      server.println(buffData[i], HEX);
    }
    // //server.print(F("     "));
    // //server.println(buffData);

    // server.println(outParse);
    value = strtoul(outParse.c_str(), NULL, 16);
    dtostrf(ConvertB32ToFloat(value), 1, 0, outKWHTOT);
    server.println(outKWHTOT);
    outParse.clear();

    break;
  }
  memset(charData, 0x00, MAX_RX_CHARS); // array is reset to 0s.
  memset(buffData, 0x00, MAX_KWH_CHAR);
}

void kwh_readRegister()
{
  switch (regStep_)
  {
  case kwhRegisterStep::KWHA:
    // server.println(F("READ KWHA"));
    kwh_parseData();
    step_ = kwhStep::SendRegister;
    regStep_ = kwhRegisterStep::KWHB;
    break;
  case kwhRegisterStep::KWHB:
    // server.println(F("READ KWHB"));
    kwh_parseData();
    step_ = kwhStep::SendRegister;
    regStep_ = kwhRegisterStep::KWHTOTAL;
    break;
  case kwhRegisterStep::KWHTOTAL:
    // server.println(F("READ KWHTOTAL"));
    kwh_parseData();
    step_ = kwhStep::AfterData;
    break;
  }
}

void kwh_sendRegister()
{
  switch (regStep_)
  {
  case kwhRegisterStep::KWHA:
    // server.println(F("SEND REGISTER KWHA"));
    cmd_readRegister(regKWHA);
    Serial2.flush();
    delay(100);
    step_ = kwhStep::ReadRegister;
    break;
  case kwhRegisterStep::KWHB:
    // server.println(F("SEND REGISTER KWHB"));
    cmd_readRegister(regKWHB);
    Serial2.flush();
    delay(100);
    step_ = kwhStep::ReadRegister;
    break;
  case kwhRegisterStep::KWHTOTAL:
    // server.println(F("SEND REGISTER KWH TOTAL"));
    cmd_readRegister(regKWHTOT);
    Serial2.flush();
    delay(100);
    step_ = kwhStep::ReadRegister;
    break;
  }
}

void kwh_sendLogout()
{
  // server.println(F("SEND LOGOUT"));
  cmd_logoffMeter();
  kwh_readACK();
}

void kwh_lastStep()
{
  // server.println(F("KWH LOGOUT -> ACK"));
  return kwh_change_status(kwhStatus::Ok);
}

void kwh_loop()
{
  if (status_ != kwhStatus::Busy)
    return;

  switch (step_)
  {
  case kwhStep::Ready: /* nothing to do, this should never happen */
    break;
  case kwhStep::Started:
    // kwh_SendConnect();
    kwh_sendLogin();
    break;
  case kwhStep::Connected:
    kwh_readACK();
    break;
  case kwhStep::SendLogin:
    kwh_sendLogin();
    break;
  case kwhStep::LoginACK:
    kwh_readACK();
    break;
  case kwhStep::SendRegister:
    kwh_sendRegister();
    break;
  case kwhStep::ReadRegister:
    kwh_readRegister();
    break;
  case kwhStep::AfterData:
    kwh_lastStep();
    break;
  case kwhStep::LogoutACK:
    kwh_lastStep();
    break;
  }
}

bool kwh_getSerialNumber()
{
  bool getSerialSuccess = false;
  // server.print(F("kwh_getSerialNumber() -> "));
  cmd_logonMeter();
  char charACK[MAX_ACK_CHAR];
  char charData[MAX_RX_CHARS];
  char flushRX;
  if (Serial2.available() > 0)
  {
    flushRX = Serial2.read();
  }
  size_t len = Serial2.readBytesUntil(ETX, charACK, MAX_ACK_CHAR);
  // server.println(len);
  // server.println(charACK[0]);
  if (charACK[1] == CAN)
  {
    // server.println(F("CAN CANCEL."));
    getSerialSuccess = false;
  }
  else if (charACK[1] == ACK)
  {
    // server.println(F("ACK RECEIVED"));
    // server.println(F("      READ  - SERIAL NUMBER."));
    cmd_readRegister(regSerNum);
    if (Serial2.available() > 0)
    {
      flushRX = Serial2.read();
    }
    size_t lens = Serial2.readBytesUntil(ETX, charData, MAX_RX_CHARS);
    // server.println(lens);
    // server.println(charData[1]);
    if (charData[1] == CHAR_REGREAD)
    {
      // server.println(F("     DATA SERIAL NUMBER"));
      for (size_t i = 0; i < SERIALNUM_CHAR; i++)
      {
        outSerialNumber[i] = charData[i + 5];
        // server.print(F("     DATA ke "));
        // server.print(i);
        // server.print(F(" > "));
        // server.println(outSerialNumber[i], HEX);
      }
      // server.print(F("     "));
      // server.println(outSerialNumber);
      //    break;
      getSerialSuccess = true;
    }
    else
      getSerialSuccess = false;
  }
  return getSerialSuccess;
}

void backgroundTask(void)
{
  timeClient.update();

  if (!mqttClientEth.loop()) /* PubSubClient::loop() returns false if not connected */
  {
    mqttConnectEth();
  }

  if (enableLora)
    lora.update();
  portal.handleClient();
}

bool mode_checked = false;
bool getSerialSuccess = false;
bool kwhReadReady = false;
bool kwhSend = false;
// NTP SECOND
void BackgroundDelay()
{
  // //server.println(timeClient.getFormattedTime());
  // if (timeClient.getSeconds() == 5)
  if (millis() - previousMillis > interval)
  {
    kwh_ready();
    kwhReadReady = true;
    kwhSend = true;
    previousMillis = millis();
  }
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(pinLED, OUTPUT);

#if defined(ARDUINO_ARCH_ESP8266)
  FlashFS.begin();
#elif defined(ARDUINO_ARCH_ESP32)
  FlashFS.begin(true);
#endif
  //
  // //server.println(enableLora);
  // if (enableLora)
  // {
  //   //server.println("ENABLE LORA");
  if (!lora.init())
  {
    // server.println("RFM95 not detected");
  }
  // loraConf();
  // }
  // else if (enableLAN)
  // {
  // //server.println("ENABLE LAN");
  Ethernet.init(csEth2);
  // }

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
  { // //server.println("load error");
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

  // //server.print("WiFi ");
  if (portal.begin())
  {
    //      //server.print("IP:" + WiFi.localIP().toString());
    config.bootUri = AC_ONBOOTURI_HOME;
  }
  else
  {
  }
  checkSendMode();
  WiFiWebServer &webServer = portal.host();
  webServer.on("/", handleRoot);
  webServer.on(AUX_RESET_URI, handleReset);
  webServer.on(AUX_REBOOT_URI, handleReboot);

  if (kwhID.length() == 0)
  {
    getSerialSuccess = kwh_getSerialNumber();
    if (getSerialSuccess)
      kwhID = String(outSerialNumber);
    else
      return;
    // server.println(kwhID);
  }

} // setup

void loop()
{
  static uint32_t next_delay = READ_DELAY;

  bool read_just_completed = false;
  backgroundTask();

  kwh_loop();
  kwhStatus status = status_;

  if (status == kwhStatus::Ready and kwhReadReady /*and timeClient.getSeconds() == 5*/)
  {
    // server.println(F("READ STATUS::READY"));
    kwh_start_reading();
  }
  else if (status == kwhStatus::Ok)
  {
    // //server.println(F("READ STATUS::OK"));

    read_just_completed = true; /* Read completed successfully */
    next_delay = READ_DELAY;    /* Reset delay to default */
    kwhReadReady = false;
    if (kwhSend)
    {
      // server.print(F("Data Akhir - "));
      // server.print(F("customerID - "));
      // server.print(customerID);
      // server.print(F("kwhID - "));
      // server.print(kwhID);
      // server.print(F("outKWHA - "));
      // server.print(outKWHA);
      // server.print(F("outKWHB - "));
      // server.print(outKWHB);
      // server.print(F("outKWHTOT - "));
      // server.println(outKWHTOT);
      String dataKirim = "*" + customerID + "*" + kwhID + "*" + String(outKWHA) + "*" + String(outKWHB) + "*" + String(outKWHTOT) + "#";

      if (enableLAN)
        mqttPublish(dataKirim);
      if (enableLora)
      {
        char myStr[50];
        dataKirim.toCharArray(myStr, sizeof(myStr));
        lora.sendUplink(myStr, strlen(myStr), 0, 1);
      }
      kwhSend = false;
    }
    status = kwhStatus::Ready;
  }
  else if (status != kwhStatus::Busy) /* Not Ready, Ok or Busy => error */
  {
    read_just_completed = true; /* Read completed with error */
    next_delay *= 2;
    if (next_delay > 60 * 1000)
    {
      next_delay = 60 * 1000;
    }
    // //server.println(F("READ STATUS::BUSY"));

    // kwh_ready();
  }

  if (!read_just_completed) // read not complete
    return;

  size_t successes = successes_;
  size_t errors = errors_;
  size_t checksum_errors = checksum_errors_;

  digitalWrite(LED_BUILTIN, HIGH);

  BackgroundDelay();
  if (status == kwhStatus::Ok)
    digitalWrite(LED_BUILTIN, LOW);

  // BackgroundDelay(5);

} // loop
