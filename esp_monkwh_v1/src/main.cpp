#include <Arduino.h>
#include <SPI.h>
#include <lorawan.h>
#include <EthernetSPI2.h>
#include <EthernetUdp.h>
#include <NTPClient.h>
#include <PubSubClient.h>
#include <AutoConnect.h>
#include <FS.h>
#include <EDMICmdLine.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#define GET_CHIPID() ((uint16_t)(ESP.getEfuseMac() >> 32))
#endif

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

#if defined(ARDUINO_ARCH_ESP32)
#include <SPIFFS.h>
fs::SPIFFSFS &FlashFS = SPIFFS;
#endif

#if defined(ARDUINO_ARCH_ESP32)
typedef WebServer WiFiWebServer;
#endif

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
EdmiCMDReader edmiread(Serial2, RXD2, TXD2);
#define MQTT_USER_ID "anyone"

//  Var CONFIG DEFAULT
String serverName;
String customerID;
String kwhID;
String kwhType;
String serverCheck;
const String channelId = "monKWH/";
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
const unsigned int interval = 5000;
const unsigned int interval_cek_konek = 2000;
const unsigned int interval_record = 10000;
long previousMillis = 0;
long previousMillis1 = 0;
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
bool flagSTX = false;
bool mode_checked = false;
bool getSerialSuccess = false;
bool kwhReadReady = false;
bool kwhSend = false;

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
      // delay(3000);
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

  mqtt_setting["mqttserver"].value = "203.194.112.238";
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
          while (true)
          {
            delay(1);
          }
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

void backgroundTask(void)
{
  portal.handleClient();
  timeClient.update();

  while (Ethernet.linkStatus() == LinkOFF)
  {
    if (checkDHCP)
    {
      Ethernet.begin(0);
    }
    else
    {
      Ethernet.begin(mac, ipStatic, ipDNS, ipGateway, ipNetmask);
    }
  }

  // if (millis() - previousMillis1 > interval_cek_konek and (!kwhSend and !kwhReadReady))
  // {
  //   kwh_Connectlive = kwh_CheckConnect();
  //   previousMillis1 = millis();
  // }
  if (!mqttClientEth.loop()) /* PubSubClient::loop() returns false if not connected */
  {
    mqttConnectEth();
  }
  edmiread.keepAlive();
  if (enableLora)
    lora.update();
}

void blink(int times)
{
  for (int x = 0; x <= times; x++)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
  }
}

void BackgroundDelay()
{
  if (timeClient.getSeconds() == 0)
  {
    kwhReadReady = true;
    edmiread.acknowledge();
    Serial.printf("TIME TO READ\n");
    previousMillis = millis();
  }
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(pinLED, OUTPUT);

#if defined(ARDUINO_ARCH_ESP32)
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
  config.apid = String(WIFI_SSID);
  config.hostName = String(WIFI_SSID);
  config.autoReset = false;
  config.autoReconnect = true;
  config.reconnectInterval = 1;
  config.portalTimeout = 5000;
  config.retainPortal = true;
  portal.config(config);
  portal.disableMenu(AC_MENUITEM_RESET | AC_MENUITEM_HOME);

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
    kwhID = edmiread.serialNumber();
    server.println(kwhID);
  }
} // setup

void loop()
{
  if (kwhID.length() == 0)
  {
    kwhID = edmiread.serialNumber();
    server.println(kwhID);
  }
  backgroundTask();
  edmiread.read_looping();
  EdmiCMDReader::Status status = edmiread.status();
  Serial.printf("%s\n", kwhReadReady ? "true" : "false");
  if (status == EdmiCMDReader::Status::Ready and kwhReadReady)
  {
    blink(1);
    Serial.println("STEP_START");
    edmiread.step_start();
  }
  else if (status == EdmiCMDReader::Status::Finish)
  {
    String allDatas = "*" +
                      customerID + "*" +
                      kwhID + "*" +
                      String(edmiread.voltR(), 4).c_str() + "*" +
                      String(edmiread.voltS(), 4).c_str() + "*" +
                      String(edmiread.voltT(), 4).c_str() + "*" +
                      String(edmiread.currentR(), 4).c_str() + "*" +
                      String(edmiread.currentR(), 4).c_str() + "*" +
                      String(edmiread.currentT(), 4).c_str() + "*" +
                      String(edmiread.wattR() / 10.0, 4).c_str() + "*" +
                      String(edmiread.wattS() / 10.0, 4).c_str() + "*" +
                      String(edmiread.wattT() / 10.0, 4).c_str() + "*" +
                      String(edmiread.pf(), 4).c_str() + "*" +
                      String(edmiread.frequency(), 4).c_str() + "*" +
                      String(edmiread.kVarh() / 1000.0, 0).c_str() + "*" +
                      String(edmiread.kwhLWBP() / 1000.0, 0).c_str() + "*" +
                      String(edmiread.kwhWBP() / 1000.0, 0).c_str() + "*" +
                      String(edmiread.kwhTotal() / 1000.0, 0).c_str() +
                      "#";

    if (enableLAN)
    {
      Serial.println("SEND WITH MQTT");
      mqttPublish(allDatas);
    }
    else if (enableLora)
    {
      char myStr[50];
      allDatas.toCharArray(myStr, sizeof(myStr));
      lora.sendUplink(myStr, strlen(myStr), 0, 1);
    }

    blink(5);

    kwhReadReady = false;
  }
  else if (status != EdmiCMDReader::Status::Busy)
  {
    Serial.println("BUSY");
    blink(1);
  }

  // digitalWrite(LED_BUILTIN, LOW);
  BackgroundDelay();

} // loop
