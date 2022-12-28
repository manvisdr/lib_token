#include <tokenonline.h>
#define BOUNDARY "--------------------------133747188241686651551404"
#define TIMEOUT 20000

//-----------API TOKEN ONLINE-------------//
const String SERVER = "203.194.112.238";
const int apiPort = 3300;
//-----------API TOKEN ONLINE-------------//

const int mqttPort = 1883;
const char *mqttChannel = "TokenOnline";
const char *MqttUser = "das";
const char *MqttPass = "mgi2022";
const String MqttChannel = "monTok/";

void MqttCallback(char *topic, byte *payload, unsigned int length)
{
  strcpy(received_topic, topic);
  memcpy(received_payload, payload, length);
  received_msg = true;
  received_length = length;
  Serial.println("void callback()");
  Serial.printf("received_topic %s received_length = %d \n", received_topic, received_length);
  Serial.printf("received_payload %s \n", received_payload);
  Serial.println("void callback()");
}

void MqttInit()
{
  mqtt.setServer(mqttServer, 1883);
  mqtt.setCallback(MqttCallback);
  mqtt.setKeepAlive(20);
  MqttTopicInit();
}

bool MqttConnect()
{
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"; // For random generation of client ID.
  char clientId[9];
  uint8_t retry = 3;
  while (!mqtt.connected())
  {
    Serial.println(String("Attempting MQTT broker:") + mqttServer);

    if (mqtt.connect(mqttChannel, MqttUser, MqttPass))
    {
      Serial.println("Established:" + String(clientId));
      mqtt.subscribe(topicREQToken);
      mqtt.subscribe(topicREQView);
      return true;
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      if (!--retry)
        break;
      // delay(3000);
    }
  }
  return false;
}

void MqttCustomPublish(char *path, String msg)
{
  mqtt.publish(path, msg.c_str());
}

void MqttTopicInit()
{
  sprintf(topicREQToken, "%s/%s/%s", mqttChannel, "request", idDevice);
  sprintf(topicREQView, "%s/%s/%s/%s", mqttChannel, "request", "view", idDevice);
  sprintf(topicRSPToken, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "token");
  sprintf(topicRSPSignal, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "signal");
  sprintf(topicRSPStatus, "%s/%s/%s/%s", mqttChannel, "respond", idDevice, "debug");

  Serial.printf("%s\n", topicREQToken);
  Serial.printf("%s\n", topicREQView);
  Serial.printf("%s\n", topicRSPSignal);
  Serial.printf("%s\n", topicRSPStatus);
}

void MqttPublishStatus(String msg)
{
  MqttCustomPublish(topicRSPStatus, msg);
}

/* *********************************** MQTT ************************* */
void MqttReconnect()
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect("monTok"))
    {
      Serial.println("connected");
      MqttPublishStatus("CONNECTED MQTT");
      mqtt.subscribe(topicREQToken);
      mqtt.subscribe(topicREQView);
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

String sendImageViewKWH(char *sn)
{
  String getAll;
  String getBody;

  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }

  // Serial.println("Connecting to server: " + mqttServer);

  if (WifiEspClient.connect(mqttServer, apiPort))
  {
    Serial.println("Connection successful!");
    MqttPublishStatus("SENDING IMAGE REQUEST BALANCE");
    Serial.println("SENDING IMAGE REQUEST BALANCE");
    String head = "--MGI\r\nContent-Disposition: form-data; name=\"imgBalance\"; filename=\"imgBalance.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--MGI--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    WifiEspClient.println("POST /saldo/response/?&sn_device=" + String(sn) + " HTTP/1.1");
    Serial.println("POST /saldo/response/?&sn_device=" + String(sn) + " HTTP/1.1");

    WifiEspClient.println("Host: " + mqttServer);
    WifiEspClient.println("Content-Length: " + String(totalLen));
    WifiEspClient.println("Content-Type: multipart/form-data; boundary=MGI");
    WifiEspClient.println();
    WifiEspClient.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024)
    {
      if (n + 1024 < fbLen)
      {
        WifiEspClient.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0)
      {
        size_t remainder = fbLen % 1024;
        WifiEspClient.write(fbBuf, remainder);
      }
    }
    WifiEspClient.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis())
    {
      Serial.print(".");
      delay(100);
      while (WifiEspClient.available())
      {
        char c = WifiEspClient.read();
        if (c == '\n')
        {
          if (getAll.length() == 0)
          {
            state = true;
          }
          getAll = "";
        }
        else if (c != '\r')
        {
          getAll += String(c);
        }
        if (state == true)
        {
          getBody += String(c);
        }
        startTimer = millis();
      }
      if (getBody.length() > 0)
      {
        break;
      }
    }
    // Serial.println();
    // WifiEspClient.stop();
    Serial.println(getBody);
  }
  else
  {
    getBody = "Connection to " + String(mqttServer) + " failed.";
    Serial.println(getBody);
  }
  return getBody;
}

String body_Nominal()
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  data += F("Content-Disposition: form-data; name=\"imgNominal\"; filename=\"picture.jpg\"\r\n");
  data += F("Content-Type: image/jpeg\r\n");
  data += F("\r\n");

  return (data);
}

String body_Saldo()
{
  String data;
  data = "--";
  data += BOUNDARY;
  data += F("\r\n");
  data += F("Content-Disposition: form-data; name=\"imgSaldo\"; filename=\"picture.jpg\"\r\n");
  data += F("Content-Type: image/jpeg\r\n");
  data += F("\r\n");

  return (data);
}

String headerNominal(char *status, size_t length)
{
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/nominal/?status=" + String(status) + "&sn_device=" + String(idDevice) + " HTTP/1.1\r\n";
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

String headerSaldo(char *token, size_t length)
{
  String tokens = String(token);
  String data;
  // data = F("POST /topup/response/?status=success&sn_device=5253252&token=33333 HTTP/1.1\r\n");
  data = "POST /topup/response/saldo/?token=" + tokens + " HTTP/1.1\r\n";
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

String sendImageNominal(char *status, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  String bodyNominal = body_Nominal();

  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodyNominal.length() + size_pic + bodyEnd.length();
  String headerTxt = headerNominal(status, allLen);

  if (!WifiEspClient.connect(mqttServer, apiPort))
  {
    return ("connection failed");
  }
  MqttPublishStatus("SENDING IMAGE NOMINAL");
  Serial.println("SENDING IMAGE NOMINAL");
  WifiEspClient.print(headerTxt + bodyNominal);
  // Serial.print(headerTxt + bodyNominal);
  WifiEspClient.write(data_pic, size_pic);

  WifiEspClient.print("\r\n" + bodyEnd);
  // Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (WifiEspClient.available())
    {
      serverRes = WifiEspClient.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  // WifiEspClient.stop();
  return serverRes;
}

String sendImageNominal(char *status, char *filename)
{
  String serverRes;
  String bodyNominal = body_Nominal();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  File myFile;
  myFile = LittleFS.open(filename); // change to your file name
  size_t filesize = myFile.size();

  size_t allLen = bodyNominal.length() + filesize + bodyEnd.length();
  String headerTxt = headerNominal(status, allLen);

  if (!WifiEspClient.connect(mqttServer, apiPort))
  {
    return ("connection failed");
  }

  MqttPublishStatus("SENDING IMAGE NOMINAL");
  Serial.println("SENDING IMAGE NOMINAL");

  WifiEspClient.print(headerTxt + bodyNominal);
  // Serial.print(headerTxt + bodyNominal);
  while (myFile.available())
    WifiEspClient.write(myFile.read());

  WifiEspClient.print("\r\n" + bodyEnd);
  // Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (WifiEspClient.available())
    {
      serverRes = WifiEspClient.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  // WifiEspClient.stop();
  myFile.close();
  return serverRes;
}

String sendImageSaldo(char *token, uint8_t *data_pic, size_t size_pic)
{
  String serverRes;
  String bodySaldo = body_Saldo();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  size_t allLen = bodySaldo.length() + size_pic + bodyEnd.length();
  String headerTxt = headerSaldo(token, allLen);

  if (!WifiEspClient.connect(mqttServer, apiPort))
  {
    return ("connection failed");
  }

  Serial.println("SENDING IMAGE SALDO");

  WifiEspClient.print(headerTxt + bodySaldo);
  Serial.print(headerTxt + bodySaldo);
  WifiEspClient.write(data_pic, size_pic);

  WifiEspClient.print("\r\n" + bodyEnd);
  Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (WifiEspClient.available())
    {
      serverRes = WifiEspClient.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  // WifiEspClient.stop();
  return serverRes;
}

String sendImageSaldo(char *token, char *filename)
{
  String serverRes;
  String bodySaldo = body_Saldo();
  String bodyEnd = String("--") + BOUNDARY + String("--\r\n");
  File myFile;
  myFile = LittleFS.open(filename); // change to your file name
  size_t filesize = myFile.size();

  size_t allLen = bodySaldo.length() + filesize + bodyEnd.length();
  String headerTxt = headerSaldo(token, allLen);

  if (!WifiEspClient.connect(mqttServer, apiPort))
  {
    return ("connection failed");
  }

  MqttPublishStatus("SENDING IMAGE SALDO");
  Serial.println("SENDING IMAGE SALDO");

  WifiEspClient.print(headerTxt + bodySaldo);
  // Serial.print(headerTxt + bodySaldo);
  while (myFile.available())
    WifiEspClient.write(myFile.read());

  WifiEspClient.print("\r\n" + bodyEnd);
  // Serial.print("\r\n" + bodyEnd);

  delay(20);
  long tOut = millis() + TIMEOUT;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    if (WifiEspClient.available())
    {
      serverRes = WifiEspClient.readStringUntil('\r');
      return (serverRes);
    }
    else
    {
      serverRes = "NOT RESPONSE";
      return (serverRes);
    }
  }
  // WifiEspClient.stop();
  myFile.close();
  return serverRes;
}

String headerNotifSaldo(char *sn)
{
  // POST /notif/lowsaldo/007 HTTP/1.1
  // Accept: */*
  // User-Agent: Thunder Client (https://www.thunderclient.com)
  // Content-Type: multipart/form-data; boundary=---011000010111000001101001
  // Host: 203.194.112.238:3300

  String serialnum = String(sn);
  String data;
  data = "POST /notif/lowsaldo/" + serialnum + " HTTP/1.1\r\n";
  data += "Accept: */*\r\n";
  data += "User-Agent: PostmanRuntime/6.4.1\r\n";
  data += "Content-Type: multipart/form-data; boundary=---011000010111000001101001";
  data += "\r\n";
  data += "Host: ";
  data += SERVER + ":" + String(apiPort);
  data += "\r\n";
  data += "\r\n";
  data += "\r\n";
  return (data);
}

String sendNotifSaldo(char *sn)
{
  String serverRes;
  String headerTxt = headerNotifSaldo(sn);

  if (!WifiEspClient.connect(mqttServer, apiPort))
  {
    return ("connection failed");
  }

  MqttPublishStatus("SENDING NOTIF SALDO");
  Serial.println("SENDING NOTIF SALDO");

  WifiEspClient.print(headerTxt);

  delay(20);
  long tOut = millis() + TIMEOUT;
  long startTimer = millis();
  boolean state = false;
  String getAll;
  String getBody;
  while (tOut > millis())
  {
    Serial.print(".");
    delay(100);
    while (WifiEspClient.available())
    {
      char c = WifiEspClient.read();
      if (c == '\n')
      {
        if (getAll.length() == 0)
        {
          state = true;
        }
        getAll = "";
      }
      else if (c != '\r')
      {
        getAll += String(c);
      }
      if (state == true)
      {
        getBody += String(c);
      }
      startTimer = millis();
    }
    if (getBody.length() > 0)
    {
      break;
    }
  }
  // WifiEspClient.stop();
  return getBody;
}

// const std::map<DebugStatus, std::string> METER_STATUS_MAP = {
//     {MeterReader::Status::Ready, "Ready"},
//     {MeterReader::Status::Busy, "Busy"},
//     {MeterReader::Status::Ok, "Ok"},
//     {MeterReader::Status::TimeoutError, "Timeout"},
//     {MeterReader::Status::IdentificationError, "Err-Idn-1"},
//     {MeterReader::Status::IdentificationError_Id_Mismatch, "Err-Idn-2"},
//     {MeterReader::Status::ProtocolError, "Err-Prot"},
//     {MeterReader::Status::ChecksumError, "Timeout"},
//     {MeterReader::Status::TimeoutError, "Err-Chk"}};

// std::string meterStatus()
// {
//   MeterReader::Status status = reader.status();
//   auto it = METER_STATUS_MAP.find(status);
//   if (it == METER_STATUS_MAP.end())
//     return "Unknw";
//   return it->second;
// }