// #include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

// // by default, LittleFS is used; to use SPIFFS, uncomment the following line
// #define CS_USE_SPIFFS true
// #define CONFIGSTORAGE_DEBUG true // show debug messages

// #include <ConfigStorage.h> // https://github.com/Tost69/ConfigStorage

// ConfigStorage CS("/data.json");
// DynamicJsonDocument configDoc(1024);
// StaticJsonDocument<512> doc;

// #define BUFFER_SIZE 20
// #define VERSION "1.0"
// #define FLAGHEAD 0x3C
// #define FLAGTAIL 0x3E
// #define MODEW 0x57
// #define MODER 0x52
// #define REGA 0x41
// #define REGE 0x45
// #define REGI 0x49
// #define REGK 0x4B
// #define REGL 0x4C
// #define REGM 0x4D
// #define REGR 0x52
// #define MSG_BUF_SIZE 255
// const int INTERCHAR_TIMEOUT = 50;

// struct deviceConf
// {
//   int id;
//   char status[10];
// };

// struct meterConf
// {
//   int id;
//   int serialnum;
//   char type[6];
// };

// struct loraConf
// {
//   bool enable;
//   char devaddr[8];
//   char nwkskey[32];
//   char appskey[32];
// };

// struct mqttConf
// {
//   bool enable;
//   int server;
//   int port;
//   char user[20];
//   char pass[20];
// };

// struct lanConf
// {
//   bool enable;
//   int ip;
//   int netmask;
//   int gateway;
//   int dns;
// };

// deviceConf deviceconf;
// meterConf meterconf;
// loraConf loraconf;
// mqttConf mqttconf;
// lanConf lanconf;

// // Buffer for incoming data
// char serial_buffer[BUFFER_SIZE];
// int buffer_position;

// // int ipStringToInt(const char *pDottedQuad, unsigned int *pIpAddr)
// // {
// //   unsigned int byte3;
// //   unsigned int byte2;
// //   unsigned int byte1;
// //   unsigned int byte0;
// //   char dummyString[2];

// //   /* The dummy string with specifier %1s searches for a non-whitespace char
// //    * after the last number. If it is found, the result of sscanf will be 5
// //    * instead of 4, indicating an erroneous format of the ip-address.
// //    */
// //   if (sscanf(pDottedQuad, "%u.%u.%u.%u%1s", &byte3, &byte2, &byte1, &byte0,
// //              dummyString) == 4)
// //   {
// //     if ((byte3 < 256) && (byte2 < 256) && (byte1 < 256) && (byte0 < 256))
// //     {
// //       *pIpAddr = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + byte0;

// //       return 1;
// //     }
// //   }

// //   return 0;
// // }

// // void printIpFromInt(unsigned int ip)
// // {
// //   unsigned char bytes[4];
// //   char buff[20];
// //   bytes[0] = ip & 0xFF;
// //   bytes[1] = (ip >> 8) & 0xFF;
// //   bytes[2] = (ip >> 16) & 0xFF;
// //   bytes[3] = (ip >> 24) & 0xFF;
// //   sprintf("%d.%d.%d.%d\n", buff, bytes[3], bytes[2], bytes[1], bytes[0]);
// //   Serial.println(buff);
// // }

// void sendMessagePacket(char message[])
// {
//   int index = 0;
//   byte checksum = 0;

//   Serial.print((char)FLAGHEAD);
//   while (message[index] != '\0')
//   {
//     Serial.print(message[index]);
//   }
//   Serial.print((char)FLAGTAIL);
// }

// boolean readChar(unsigned int timeout, byte *inChar)
// {
//   unsigned long currentMillis = millis();
//   unsigned long previousettingsillis = currentMillis;

//   while (currentMillis - previousettingsillis < timeout)
//   {
//     if (Serial.available())
//     {
//       (*inChar) = (byte)Serial.read();
//       return true;
//     }
//     currentMillis = millis();
//   } // while
//   return false;
// }

// boolean readMessage(char *message, int maxMessageSize, unsigned int timeout)
// {
//   byte inChar;
//   int index = 0;
//   boolean completeMsg = false;

//   unsigned long currentMillis = millis();
//   unsigned long previousettingsillis = currentMillis;

//   while (currentMillis - previousettingsillis < timeout)
//   {
//     if (Serial.available())
//     {
//       inChar = (byte)Serial.read();

//       if (inChar == FLAGHEAD) // start of message?
//       {
//         while (readChar(INTERCHAR_TIMEOUT, &inChar))
//         {
//           if (inChar != FLAGTAIL)
//           {
//             message[index] = inChar;
//             index++;
//             if (index == maxMessageSize) // buffer full
//               break;
//           }
//           else // got FLAGTAIL, next character is the checksum
//           {
//             message[index] = '\0';
//             completeMsg = true;
//             break;
//           }
//         } // while read until FLAGTAIL or timeout
//       }   // inChar == FLAGHEAD
//     }     // Serial.available()
//     if (completeMsg)
//       break;
//     currentMillis = millis();
//   } // while
//   return completeMsg;
// }

// void parseEth(char *str)
// {
//   const char s[2] = "|";
//   char *token;

//   /* get the first token */
//   token = strtok(str, s);

//   /* walk through other tokens */
//   while (token != NULL)
//   {
//     Serial.println(token);

//     token = strtok(NULL, s);
//   }
// }

// void setEth()
// {
// }

// void setup()
// {
//   Serial.begin(9600);
//   while (!Serial)
//     ;

//   delay(200);
//   // CS.print();
//   doc = CS.get();
//   // save configuration as file
//   // CS.set(configDoc);
//   CS.save();

//   // configDoc["device"]["id"] = 909090;
//   // CS.set(configDoc);
//   // CS.save();

//   // CS.print();

//   // char buffer[255];
//   // serializeJsonPretty(doc, buffer);
//   // Serial.println(buffer);
// }

// void readAll()
// {
//   // change device config value
//   deviceconf.id = doc["device"]["id"];
//   strlcpy(deviceconf.status,          // <- destination
//           doc["device"]["status"],    // <- source
//           sizeof(deviceconf.status)); // <- destination's capacity

//   // change meter config value
//   meterconf.id = doc["meter"]["id"];
//   meterconf.serialnum = doc["meter"]["serialnum"];
//   strlcpy(meterconf.type,          // <- destination
//           doc["meter"]["type"],    // <- source
//           sizeof(meterconf.type)); // <- destination's capacity

//   // change lora config value
//   loraconf.enable = doc["lora"]["boolean"];
//   strlcpy(loraconf.devaddr,          // <- destination
//           doc["lora"]["devaddr"],    // <- source
//           sizeof(loraconf.devaddr)); // <- destination's capacity
//   strlcpy(loraconf.nwkskey,          // <- destination
//           doc["lora"]["nwkskey"],    // <- source
//           sizeof(loraconf.nwkskey)); // <- destination's capacity
//   strlcpy(loraconf.appskey,          // <- destination
//           doc["lora"]["appskey"],    // <- source
//           sizeof(loraconf.appskey)); // <- destination's capacity

//   // change mqtt config value
//   mqttconf.enable = doc["mqtt"]["boolean"];
//   mqttconf.server = doc["mqtt"]["server"];
//   mqttconf.port = doc["mqtt"]["port"];
//   strlcpy(mqttconf.user,          // <- destination
//           doc["mqtt"]["user"],    // <- source
//           sizeof(mqttconf.user)); // <- destination's capacity
//   strlcpy(mqttconf.pass,          // <- destination
//           doc["mqtt"]["pass"],    // <- source
//           sizeof(mqttconf.pass)); // <- destination's capacity

//   // change lan config value
//   lanconf.enable = doc["lan"]["boolean"];
//   lanconf.ip = doc["lan"]["server"];
//   lanconf.netmask = doc["lan"]["port"];
//   lanconf.gateway = doc["lan"]["gateway"];
//   lanconf.dns = doc["lan"]["dns"];

//   Serial.printf("I|%d|%s| ", deviceconf.id, deviceconf.status);
//   Serial.printf("K|%d|%d|%s| ", meterconf.id, meterconf.serialnum, meterconf.type);
//   Serial.printf("L|%d|%s|%s|%s| ", loraconf.enable, loraconf.devaddr, loraconf.nwkskey, loraconf.appskey);
//   Serial.printf("M|%d|%d|%d|%s|%s| ", mqttconf.enable, mqttconf.server, mqttconf.port, mqttconf.user, mqttconf.pass);
//   Serial.printf("E|%d|%d|%d|%d|%d| >", lanconf.enable, lanconf.ip, lanconf.netmask, lanconf.gateway, lanconf.dns);
// }

// void loop()
// {
//   char msg[MSG_BUF_SIZE];

//   if (readMessage(msg, MSG_BUF_SIZE, 5000))
//   {
//     switch (msg[0])
//     {
//     case MODER:
//       switch (msg[1])
//       {
//       case REGA:
//         readAll();
//         // Serial.println("readall");
//         break;
//       case REGE:
//         // parseEth(msg);
//         Serial.println("E192.168.1.1|255.255.255.0|192.168.1.1|8.8.8.8");
//         break;
//       case REGI:
//         Serial.println("I696969696");
//         break;
//       case REGK:
//         Serial.println("");
//         break;
//       case REGL:
//         Serial.println("");
//         break;
//       case REGM:
//         Serial.println("");
//         break;
//       case REGR:
//         Serial.println("");
//         break;
//       default:
//         break;
//       }
//       break;
//     case MODEW:
//       switch (msg[1])
//       {
//       case REGA:
//         // readAll();
//         Serial.print(msg);
//         break;
//       case REGE:
//         // parseEth(msg);
//         Serial.println("E192.168.1.1|255.255.255.0|192.168.1.1|8.8.8.8");
//         break;
//       case REGI:
//         Serial.println("I696969696");
//         break;
//       case REGK:
//         Serial.println("");
//         break;
//       case REGL:
//         Serial.println("");
//         break;
//       case REGM:
//         Serial.println("");
//         break;
//       case REGR:
//         Serial.println("");
//         break;
//       default:
//         break;
//       }
//       break;
//     default:
//       break;
//     }
//   }
// }

#include "Macro.h"
#include "SettingsManager.h"
#include <Arduino.h>

#include <SerialCommand.h>
SerialCommand SCmd; // The demo SerialCommand object

SettingsManager settings;
IPAddress ipNew;
const char *configFile = "/config.json";

char devSerNum[20];
char devType[5];
float devFirmVer;
int devRecord;
float GpsLat;
float GpsLong;
char devStatus[10];

char meterSernum[25];
char meterType[5];
int meterV_scale;
int meterI_scale;
int meterP_scale;
int meterKvar_scale;
int meterKwh_scale;

bool loraEnable;
int loraDevAddr;
char loraNwkSKey[33];
char loraAppSKey[33];
bool debugBool = false;

void getTests()
{
  // put your setup code here, to run once:
  settings.readSettings("/config.json");

  Serial.println("Device ID : " + String(settings.getInt("device.id")));             //"id":455544
  Serial.println("Device Type : " + String(settings.getChar("device.type")));        //"type": "LORA",
  Serial.println("Device GpsLat : " + String(settings.getFloat("device.gpslat")));   //"gpslat": -7.3439796,
  Serial.println("Device GpsLong : " + String(settings.getFloat("device.gpslong"))); //"gpslong": 112.7522739,
  Serial.println("Device Recording : " + String(settings.getInt("device.record")));  //"record": 60,
  Serial.println("Device Status : " + String(settings.getChar("device.status")));    //"status": "running"

  Serial.println("Meter ID : " + String(settings.getInt("meter.id")));                 //   "id": 2424242,
  Serial.println("Meter serialnum : " + String(settings.getInt("meter.serialnum")));   //   "serialnum": 12414314,
  Serial.println("Meter type : " + String(settings.getChar("meter.type")));            //   "type": "MK10E",
  Serial.println("Meter v_scale : " + String(settings.getInt("meter.v_scale")));       //   "v_scale": 1,
  Serial.println("Meter i_scale : " + String(settings.getInt("meter.i_scale")));       //   "i_scale": 1,
  Serial.println("Meter p_scale : " + String(settings.getInt("meter.p_scale")));       //   "p_scale": 1,
  Serial.println("Meter kvar_scale : " + String(settings.getInt("meter.kvar_scale"))); //   "kvar_scale": 1,
  Serial.println("Meter kwh_scale : " + String(settings.getInt("meter.kwh_scale")));   //   "kwh_scale": 1

  Serial.println("Lora enable : " + String(settings.getBool("lora.enable")));   //   "enable": true,
  Serial.println("Lora devaddr : " + String(settings.getInt("lora.devaddr")));  //   "devaddr": "260D6ECC",
  Serial.println("Lora nwkskey : " + String(settings.getChar("lora.nwkskey"))); //   "nwkskey": "1FD053B476BF4F732DC8CF5E565538B9",
  Serial.println("Lora appskey : " + String(settings.getChar("lora.appskey"))); //   "appskey": "2EE8E2940E6799AD78F9F4C6DA8C53D1"

  Serial.println("Mqtt enable : " + String(settings.getBool("mqtt.enable"))); //  "enable": true,
  Serial.println("Mqtt server : " + String(settings.getInt("mqtt.server")));  //  "server": 17435146,
  Serial.println("Mqtt port : " + String(settings.getInt("mqtt.port")));      //  "port": 1883,
  Serial.println("Mqtt user : " + String(settings.getChar("mqtt.user")));     //  "user": "mgiadmin",
  Serial.println("Mqtt pass : " + String(settings.getChar("mqtt.pass")));     //  "pass": "1234567890"

  Serial.println("Lan enable : " + String(settings.getBool("lan.enable")));  //  "boolean": true,
  Serial.println("Lan ip : " + String(settings.getInt("lan.ip")));           //  "ip": 17435146,
  Serial.println("Lan netmask : " + String(settings.getInt("lan.netmask"))); //  "netmask": 17435146,
  Serial.println("Lan gateway : " + String(settings.getInt("lan.gateway"))); //  "gateway": 17435146,
  Serial.println("Lan dns : " + String(settings.getInt("lan.dns")));         //  "dns": 17435146
}

bool SettingsInit()
{
  settings.readSettings(configFile);

  strcpy(devSerNum, settings.getChar("device.serialnum"));
  strcpy(devType, settings.getChar("device.type"));
  devFirmVer = settings.getFloat("device.firmwareversion");

  GpsLat = settings.getFloat("device.gpslat");
  GpsLong = settings.getFloat("device.gpslong");
  strcpy(devStatus, settings.getChar("device.status"));
  devRecord = settings.getInt("device.record");

  strcpy(meterSernum, settings.getChar("meter.sernum"));
  strcpy(meterType, settings.getChar("meter.type"));
  meterV_scale = settings.getInt("meter.v_scale");
  meterI_scale = settings.getInt("meter.i_scale");
  meterP_scale = settings.getInt("meter.p_scale");
  meterKvar_scale = settings.getInt("meter.kvar_scale");
  meterKwh_scale = settings.getInt("meter.kwh_scale");

  loraEnable = settings.getBool("lora.enable");
  loraDevAddr = settings.getInt("lora.devaddr");
  strcpy(loraNwkSKey, settings.getChar("lora.nwkskey"));
  strcpy(loraAppSKey, settings.getChar("lora.appskey"));

  Serial.println("Device ID : " + String(devSerNum));
  Serial.println("Device ID : " + String(devType));
  Serial.println("Device ID : " + String(devFirmVer));
  Serial.println("Device GpsLat : " + String(GpsLat));       //"gpslat": -7.3439796,
  Serial.println("Device GpsLong : " + String(GpsLong));     //"gpslong": 112.7522739,
  Serial.println("Device Recording : " + String(devRecord)); //"record": 60,
  Serial.println("Device Status : " + String(devStatus));    //"status": "running"

  Serial.println("Meter serialnum : " + String(meterSernum));
  Serial.println("Meter type : " + String(meterType));             //   "type": "MK10E",
  Serial.println("Meter v_scale : " + String(meterV_scale));       //   "v_scale": 1,
  Serial.println("Meter i_scale : " + String(meterI_scale));       //   "i_scale": 1,
  Serial.println("Meter p_scale : " + String(meterP_scale));       //   "p_scale": 1,
  Serial.println("Meter kvar_scale : " + String(meterKvar_scale)); //   "kvar_scale": 1,
  Serial.println("Meter kwh_scale : " + String(meterKwh_scale));   //   "kwh_scale": 1

  Serial.println("Lora enable : " + String(loraEnable));
  Serial.println("Lora devaddr : " + String(loraDevAddr)); //   "devaddr": "260D6ECC",
  Serial.println("Lora nwkskey : " + String(loraNwkSKey)); //   "nwkskey": "1FD053B476BF4F732DC8CF5E565538B9",
  Serial.println("Lora appskey : " + String(loraAppSKey)); //   "appskey": "2EE8E2940E6799AD78F9F4C6DA8C53D1"

  return true;
}

void SettingsFromMeterEdmi()
{

  // Serial.print("WriteResult:");
  // Serial.println(ip);
  // settings.writeSettings("/config.json");
}

void SettingsPrint()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  File file2 = LittleFS.open(configFile);

  if (!file2)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("File Content:");

  while (file2.available())
  {

    Serial.write(file2.read());
  }

  file2.close();
}

void SerialSettingReadAll()
{
  // Serial.printf("<I|%d|%s|%s|%.1f|%.1f|%.1f|%s|%s| ",
  // devId, devSerNum, devType, devFirmVer, GpsLat, GpsLong, devRecord, devStatus);
  Serial.printf("<I|%s|%s|%.1f|%s|%.1f|%.1f| ",
                devSerNum, devType, devFirmVer, devStatus, GpsLat, GpsLong);
  Serial.printf("K|%s|%s|%d|%d|%d|%d|%d|%d|%d|%d| ",
                meterSernum, meterType, devRecord, meterV_scale, meterI_scale, meterP_scale,
                meterKvar_scale, meterKvar_scale, meterKwh_scale, meterKwh_scale);
  Serial.printf("L|%d|%d|%s|%s|>" /* >" */, loraEnable, loraDevAddr, loraNwkSKey, loraAppSKey);
  // Serial.printf("M|%d|%d|%d|%s|%s| ", mqttconf.enable, mqttconf.server, mqttconf.port, mqttconf.user, mqttconf.pass);
  // Serial.printf("E|%d|%d|%d|%d|%d| >", lanconf.enable, lanconf.ip, lanconf.netmask, lanconf.gateway, lanconf.dns);
}

void LED_off()
{
  Serial.println("LED_off");
}

void process_command()
{
  int aNumber;
  char *arg;

  Serial.println("We're in process_command");
  arg = SCmd.next();
  if (arg != NULL)
  {
    aNumber = atoi(arg); // Converts a char string to an integer
    Serial.print("First argument was: ");
    Serial.println(aNumber);
  }
  else
  {
    Serial.println("No arguments");
  }

  arg = SCmd.next();
  if (arg != NULL)
  {
    aNumber = atol(arg);
    Serial.print("Second argument was: ");
    Serial.println(aNumber);
  }
  else
  {
    Serial.println("No second argument");
  }
}

void DebugOn()
{
  debugBool = true;
  while (debugBool)
  {
    Serial.println("<5|*1*414535*12.53535*343342#|>");
    delay(1000);
  }
}

void DebugOff()
{
  debugBool = false;
}

void setup()
{
  Serial.begin(115200);
  SCmd.addCommand("<RALL>", SerialSettingReadAll); // Turns LED on
  SCmd.addCommand("<LORA>", LED_off);              // Turns LED off
  SCmd.addCommand("<P>", process_command);         // Converts two arguments to integers and echos them back
  // SCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?")
  SCmd.addCommand("<DBUGON>", DebugOn);
  SCmd.addCommand("<DBUGOFF>", DebugOff);
  Serial.println("Ready");

  char version[55] = {0};

  SettingsInit();
  // strcpy(meterSernum,settings.getChar("meter.serialnum"));
  // int ip = settings.setInt("lan.ip", 12345678);
  // Serial.print("WriteResult:");
  // Serial.println(ip);
  // settings.writeSettings("/config.json");
}

void loop()
{
  SCmd.readSerial();
}
