/*
**  Program   : ESP_SysLogger
*/
#define _FW_VERSION "v2.0.1 (20-12-2022)"
/*
**  Copyright (c) 2019 .. 2023 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.
***************************************************************************/

#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPPerfectTime.h>
#include <TimeLib.h> // https://github.com/PaulStoffregen/Time
#include <PubSubClient.h>

#include "SPIFFS_SysLogger.h"
#include "esp_log.h"
#define _FSYS SPIFFS
using ssid_char_t = char;
ssid_char_t *ssid = "MGI-MNC";
ssid_char_t *password = "#neurixmnc#";
ESPSL sysLog; // Create instance of the ESPSL object
WiFiClient client;
IPAddress ipserver(203, 194, 112, 238);
PubSubClient mqtt(client);

const String SERVER = "203.194.112.238";
// const String SERVER = "192.168.0.190";
int PORT = 3300;
#define BOUNDARY "--------------------------133747188241686651551404"
/*
** you can add your own debug information to the log text simply by
** defining a macro using ESPSL::writeDbg( ESPSL::buildD(..) , ..)
*/
#if defined(_Time_h)
/* example of debug info with time information */
#define writeToSysLog(...) ({ sysLog.writeDbg(sysLog.buildD("[%02d:%02d:%02d][%-12.12s] ", hour(), minute(), second(), __FUNCTION__), __VA_ARGS__); })
#else
/* example of debug info with calling function and line in calling function */
#define writeToSysLog(...) ({ sysLog.writeDbg(sysLog.buildD("(%4d)[%-12.12s(%4d)] ", number++, __FUNCTION__, __LINE__), __VA_ARGS__); })
#endif

uint32_t statusTimer;
uint32_t number = 0;
const char *ntpServer = "0.id.pool.ntp.org";

//-------------------------------------------------------------------------
void listFileSys(bool doSysLog)
{
  Serial.println("===============================================================");

  {
    File root = _FSYS.open("/");
    File file = root.openNextFile();
    while (file)
    {
      String fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("FS File: %s, size: %d\r\n", fileName.c_str(), fileSize);
      if (doSysLog)
      {
        writeToSysLog("FS File: %s, size: %d\r\n", fileName.c_str(), fileSize);
      }
      file = root.openNextFile();
    }
  }
  Serial.println("===============================================================");
} // listFileSys()

//-------------------------------------------------------------------------
void showBareLogFile()
{
  Serial.println("\n=====================================================");
  // writeToSysLog("Dump logFile [sysLog.dumpLogFile()]");
  sysLog.dumpLogFile();

} // showBareLogFile()

//-------------------------------------------------------------------------
void testReadnext()
{
  char lLine[200] = {0};
  int lineCount;

  Serial.println("testing startReading() & readNextLine() functions...");

  Serial.println("\n=====from oldest for n lines=========================");
  Serial.println("sysLog.startReading()");
  sysLog.startReading();
  Serial.println("sysLog.readNextLine()");
  lineCount = 0;
  while (sysLog.readNextLine(lLine, sizeof(lLine)))
  {
    Serial.printf("[%3d]==>> %s \r\n", ++lineCount, lLine);
  }
  Serial.println("\n=====from newest for n lines=========================");
  Serial.println("sysLog.startReading()");
  sysLog.startReading();
  Serial.println("sysLog.readPreviousLine()");
  lineCount = 0;
  while (sysLog.readPreviousLine(lLine, sizeof(lLine)))
  {
    Serial.printf("[%3d]==>> %s \r\n", ++lineCount, lLine);
  }

  Serial.println("\nDone testing readNext()\n");

} // testReadNext()

//-------------------------------------------------------------------------
void dumpSysLog()
{
  int seekVal = 0, len = 0;
  char cIn;

  File sl = _FSYS.open("/log.txt", "r");
  Serial.println("\r\n===charDump=============================");
  Serial.printf("%4d [", 0);
  while (sl.available())
  {
    cIn = (char)sl.read();
    len++;
    seekVal++;
    if (cIn == '\n')
    {
      Serial.printf("] (len [%d])\r\n", len);
      len = 0;
      Serial.printf("%4d [", seekVal);
    }
    else
      Serial.print(cIn);
  }
  sl.close();
  Serial.println("\r\n========================================\r\n");

} //  dumpSysLog()

void connectWiFi()
{
  WiFi.begin(ssid, password);

  Serial.print("\nWifi connecting...");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("connected. ");
}

//-------------------------------------------------------------------------

void printTime(struct tm *tm, suseconds_t usec)
{
  Serial.printf("%04d/%02d/%02d %02d:%02d:%02d.%06ld\n",
                tm->tm_year + 1900,
                tm->tm_mon + 1,
                tm->tm_mday,
                tm->tm_hour,
                tm->tm_min,
                tm->tm_sec,
                usec);
}

// timeToChar(time,second,&char)
void timeToChar(struct tm *tm, suseconds_t usec, char *dest)
{
  sprintf(dest, "[%04d/%02d/%02d %02d:%02d:%02d.%06ld]",
          tm->tm_year + 1900,
          tm->tm_mon + 1,
          tm->tm_mday,
          tm->tm_hour,
          tm->tm_min,
          tm->tm_sec,
          usec);
}

void DuplicatingFile(char *sourceFilename, char *destFilename)
{
  if (SPIFFS.exists(destFilename))
  {
    Serial.printf("File %s exits\n", destFilename);
  }
  else
  {
    File file = SPIFFS.open(destFilename, FILE_WRITE, true);
    Serial.printf("File %s created\n", destFilename);
    file.close();
  }
  File filenow = SPIFFS.open(sourceFilename, "r");
  File fileprev = SPIFFS.open(destFilename, FILE_WRITE);
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
  Serial.printf("DUPLICATING /log_now.txt to %s SUCCESS\n", destFilename);
}

bool RemovingFile(char *filename)
{
  bool stateFile;
  if (SPIFFS.exists(filename))
  {
    Serial.printf("File Daily log %s exits\n", filename);
  }
  else
    return false;

  stateFile = SPIFFS.remove(filename);

  return stateFile;
}

const int payloadSize = 50;
const int topicSize = 64;
char received_topic[topicSize];
char received_payload[payloadSize];
unsigned int received_length;
bool received_msg = false;
char *topic = "Dummy/001/";

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

void MqttReconnect()
{
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
  // Loop until we're reconnected
  char clientId[9];
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    for (uint8_t i = 0; i < 8; i++)
    {
      clientId[i] = alphanum[random(62)];
    }
    clientId[8] = '\0';
    if (mqtt.connect(clientId))
    {
      Serial.println("connected");
      mqtt.subscribe(topic);
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
  data = "POST /logdevice/upload/?sn=001&tgl=" + String(tgl) + " HTTP/1.1\r\n";
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
  return ("UPLOADED");
}

void setup()
{
  Serial.begin(115200);

  Serial.println("\nESP32: Start ESP System Logger ....\n");
  connectWiFi();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  _FSYS.begin(true);

  sysLog.setOutput(&Serial, 115200);
  mqtt.setServer(ipserver, 1883);
  mqtt.setCallback(callback);
  // sysLog.setDebugLvl(1);

  //--> max linesize is declared by _MAXLINEWIDTH in the
  //    library and is set @150, so 160 will be truncated to 150!
  //--> min linesize is set to @50
  //-----------------------------------------------------------------
  // use existing sysLog file or create one
  // sysLog.removeSysLog();
  if (!sysLog.begin(27, 60))
  {
    Serial.println("Error opening sysLog!\r\nCreated sysLog!");
    delay(5000);
  }
  pftime::configTzTime(PSTR("WIB-7"), ntpServer);
  sysLog.status();
  // testReadnext();

  int strtId = sysLog.getLastLineID();
  Serial.printf("Last line Log - %d", strtId);

  sysLog.setDebugLvl(3);
  sysLog.status();

  // showBareLogFile();

  // DuplicatingFile("/sysLog.dat", "/log_prev.txt");
  // if (RemovingFile("/log_prev.txt"))
  //   Serial.println("REMOVING FILE SUCCESS");

  Serial.println("\nsetup() done .. \n");
  delay(1000);

} // setup()
//-------------------------------------------------------------------------
void loop()
{
  if (!mqtt.connected() /*and status == Status::IDLE*/)
  {
    MqttReconnect();
  }

  // // testReadnext();

  // struct tm *tm = pftime::localtime(nullptr);
  // suseconds_t usec;
  // tm = pftime::localtime(nullptr, &usec);
  // printTime(tm, usec);
  // timeToChar(tm, usec, timestamps);
  // Serial.println(timestamps);
  if (received_msg and strcmp(received_topic, topic) == 0)
  {
    char timestamps[30];
    struct tm *tm = pftime::localtime(nullptr);
    suseconds_t usec;
    tm = pftime::localtime(nullptr, &usec);
    printTime(tm, usec);
    timeToChar(tm, usec, timestamps);
    int strtId = sysLog.getLastLineID();
    
    sysLog.writef("%s %-5s loop.225: %s", timestamps, "INFO", received_payload);
    received_msg = false;
  }
  mqtt.loop();
  // timeToChar(tm, usec, timestamps);
  // Serial.println(timestamps);
} // loop()
