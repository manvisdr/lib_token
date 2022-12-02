#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "FS.h"
#include "SPIFFS.h"
#include <HTTPClient.h>

WiFiClient client;
HTTPClient http;

#define FORMAT_SPIFFS_IF_FAILED true
const char *WIFI_SSID = "MGI-MNC";
const char *WIFI_PASSWORD = "#neurixmnc#";

const char *host = "203.194.112.238";
const int Port = 3300;
String boundry = "---011000010111000001101001";

String serverName = "203.194.112.238";      // REPLACE WITH YOUR Raspberry Pi IP ADDRESS OR DOMAIN NAME
String serverEndpoint = "/topup/response/"; // Needs to match upload server endpoint
String keyName = "\"file\"";                // Needs to match upload server keyName
const int serverPort = 3300;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("− failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" − not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void sendToServer(File fb)
{
  if (client.connect(serverName.c_str(), serverPort))
  {

    Serial.println("Connection successful!");

    // *** Temp filename!!! ***
    // String fileName = picname +String(picNumber) + filext;
    // String keyName = "\"myFile\"";

    // Make a HTTP request and add HTTP headers
    String boundary = "SolarCamBoundaryjg2qVIUS8teOAbN3";
    String contentType = "image/jpeg";
    String portString = String(serverPort);
    String hostString = serverName;
    String status = "sukses";
    String pelanggan = "01";
    String sn = "5865876";
    String token = "34235325252525";

    // post header
    String postHeader = "POST " + serverEndpoint + "?status=gagal&pelanggan=1&sn=11&token=33333" + " HTTP/1.1\r\n";
    postHeader += "Host: " + hostString + ":" + portString + "\r\n";
    postHeader += "Content-Type: multipart/form-data; boundary=" + boundry + "\r\n";
    postHeader += "Accept: */*\r\n";
    // postHeader += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    // postHeader += "Accept-Encoding: gzip,deflate\r\n";
    // postHeader += "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n";
    postHeader += "User-Agent: ESP32\r\n";
    // postHeader += "Keep-Alive: 300\r\n";
    // postHeader += "Connection: keep-alive\r\n";
    // postHeader += "Accept-Language: en-us\r\n";

    // request header
    String requestHead = "--" + boundry + "\r\n";
    // requestHead += "Content-Disposition: form-data; name=\"myFile\"; filename=\"" + fileName + "\"\r\n";
    requestHead += "Content-Disposition: form-data; name=" + keyName + "; filename=\"" + "file" + "\"\r\n";

    requestHead += "Content-Type: " + contentType + "\r\n\r\n";

    // request tail
    String tail = "\r\n--" + boundry + "--\r\n\r\n";

    uint32_t imageLen = fb.size();

    // content length
    // int contentLength = keyHeader.length() + requestHead.length() + imageLen + tail.length();
    int contentLength = requestHead.length() + imageLen + tail.length();
    postHeader += "Content-Length: " + String(contentLength, DEC) + "\n\n";

    // send post header
    char charBuf0[postHeader.length() + 1];
    postHeader.toCharArray(charBuf0, postHeader.length() + 1);

    client.write(charBuf0, postHeader.length());
    Serial.print(charBuf0);

    // send request buffer
    char charBuf1[requestHead.length() + 1];
    requestHead.toCharArray(charBuf1, requestHead.length() + 1);
    client.write(charBuf1, requestHead.length());
    Serial.print(charBuf1);

    while (fb.available())
    {
      client.write(fb.read());
      
    }

    char charBuf3[tail.length() + 1];
    tail.toCharArray(charBuf3, tail.length() + 1);
    client.write(charBuf3, tail.length());
    Serial.print(charBuf3);
  }
}

void setup()
{
  Serial.begin(115200);
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  listDir(SPIFFS, "/", 0);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println(F("WiFi connected"));
  Serial.println("");
  Serial.println(WiFi.localIP());

  File normPicture = SPIFFS.open("/data.jpg", FILE_READ);
  uint32_t jpgLen = normPicture.size();

  Serial.print("Sending image of ");
  Serial.print(jpgLen, DEC);
  Serial.println("bytes");

  sendToServer(normPicture);
}

void loop()
{
  // put your main code here, to run repeatedly:
}