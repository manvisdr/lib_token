#include <tokenonline.h>

long start_wifi_millis;
long wifi_timeout = 10000;
int cntLoss;
char *sPtr[20];
char *strData = NULL; // this is allocated in separate and needs to be free( ) eventually

void BluetoothCallback();
void KoneksiInit()
{
  SerialBT.begin(bluetooth_name);
  SerialBT.register_callback(BluetoothCallback);

  // xTaskCreatePinnedToCore(
  //     CheckPingBackground,
  //     "pingServer",     // Task name
  //     5000,             // Stack size (bytes)
  //     NULL,             // Parameter
  //     tskIDLE_PRIORITY, // Task priority
  //     NULL,             // Task handle
  //     0);
}

void BluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  char serialBTBuffer[50];

  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("BT Client Connected");
    KoneksiStage = BT_INIT;
  }

  if (event == ESP_SPP_DATA_IND_EVT and KoneksiStage == BT_INIT)
  {
    int N = WiFiFormParsing(SerialBT.readString(), sPtr, 20, &strData);
    if (N == 2)
    {
      strcpy(wifissidbuff, sPtr[0]);
      strcpy(wifipassbuff, sPtr[1]);
      // }
      Serial.println(wifissidbuff);
      Serial.println(wifipassbuff);
      SettingsWifiSave(wifissidbuff, wifipassbuff);
      ESP.restart();
    }

    // if (event == ESP_SPP_DATA_IND_EVT and KoneksiStage == BT_INIT)
    // {
    //   strcpy(serialBTBuffer, SerialBT.readString().c_str());
    //   if (WiFIQRCheck(serialBTBuffer))
    //   {
    //     WiFiQRParsing(serialBTBuffer, &wifissidbuff, &wifipassbuff);
    //     Serial.println(wifissidbuff);
    //     Serial.println(wifipassbuff);
    //   }
    //   else
    //     Serial.println("NOT WIFI QR");
    // }
  }
}
void WifiInit()
{
  if (strcmp(wifipassbuff, "NULL") == 0)
    WiFi.begin(wifissidbuff, NULL);
  else
    WiFi.begin(wifissidbuff, wifipassbuff);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (millis() - start_wifi_millis > wifi_timeout)
    {
      WiFi.disconnect(true, true);
    }
  }
  Serial.println("WifiInit()...Successfull");
  Serial.print("WiFi connected to");
  Serial.print(wifissidbuff);
  Serial.print(" IP: ");
  Serial.println(WiFi.localIP());

  OTASERVER.on("/", []()
               { OTASERVER.send(200, "text/plain", "Hi! I am ESP8266."); });

  ElegantOTA.begin(&OTASERVER); // Start ElegantOTA
  OTASERVER.begin();
}

char *extract_between(const char *str, const char *p1, const char *p2)
{
  const char *i1 = strstr(str, p1);
  if (i1 != NULL)
  {
    const size_t pl1 = strlen(p1);
    const char *i2 = strstr(i1 + pl1, p2);
    if (p2 != NULL)
    {
      /* Found both markers, extract text. */
      const size_t mlen = i2 - (i1 + pl1);
      char *ret = (char *)malloc(mlen + 1);
      if (ret != NULL)
      {
        memcpy(ret, i1 + pl1, mlen);
        ret[mlen] = '\0';
        return ret;
      }
    }
  }
}

void WiFiQRParsing(const char *input, char **destSSID, char **destPASS)
{
  char *s;
  s = strstr(input, ";T:"); // search for string "hassasin" in buff
  if (s != NULL)            // if successful then s now points at "hassasin"
  {
    *destSSID = extract_between(input, "WIFI:S:", ";T:");
    if (strcmp(extract_between(input, ";T:", ";P:"), "nopass") == 0)
    {
      *destPASS = NULL;
    }
    else
      *destPASS = extract_between(input, ";P:", ";H:");
  } // index of "hassasin" in buff can be found by pointer subtraction
  else
  {
    *destSSID = extract_between(input, "WIFI:S:", ";;");
    *destPASS = NULL;
  }
}

bool WiFIQRCheck(const char *input)
{
  char *s;
  s = strstr(input, "WIFI:"); // search for string "hassasin" in buff
  if (s != NULL)              // if successful then s now points at "hassasin"
    return true;
  else
    return false;
}

int WiFiFormParsing(String str, // pass as a reference to avoid double memory usage
                    char **p, int size, char **pdata)
{
  int n = 0;
  free(*pdata); // clean up last data as we are reusing sPtr[ ]
  // BE CAREFUL not to free it twice
  // calling free on NULL is OK
  *pdata = strdup(str.c_str()); // allocate new memory for the result.
  if (*pdata == NULL)
  {
    Serial.println("OUT OF MEMORY");
    return 0;
  }
  *p++ = strtok(*pdata, ";");
  for (n = 1; NULL != (*p++ = strtok(NULL, ";")); n++)
  {
    if (size == n)
    {
      break;
    }
  }
  return n;
}

void disconnect_bluetooth()
{
  delay(1000);
  Serial.println("BT stopping");
  SerialBT.println("Bluetooth disconnecting...");
  delay(1000);
  SerialBT.flush();
  SerialBT.disconnect();
  SerialBT.end();
  Serial.println("BT stopped");
  delay(1000);
  bluetooth_disconnect = false;
}

int CheckPingGateway()
{
  int pingReturn;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Ping.ping(WiFi.localIP()))
      cntLoss = 0;
    else
      cntLoss++;
  }
  if (cntLoss > 7)
    pingReturn = 0;
  else if (cntLoss = 0)
    pingReturn = 100;
  else
    pingReturn = 50;
  return cntLoss;
}

void CheckPingServer()
{
  int pingReturn;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Ping.ping(mqttServer), 2)
      cntLoss = 0;
    else
      cntLoss++;
  }
  Serial.printf("CheckPingServer() : cntLoss=%d\n", cntLoss);
}

void CheckPingBackground(void *parameter)
{
  pingTime = Alarm.timerRepeat(10, CheckPingServer);
  pingTimeSend = Alarm.timerRepeat(30, CheckPingSend);
  for (;;)
  {
    // Serial.print("Main Ping: Executing on core ");
    // Serial.println(xPortGetCoreID());
    Alarm.delay(1000);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void CheckPingSend()
{
  if (cntLoss == 3)
  {
    MqttCustomPublish(topicRSPSignal, "1");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 1"));
    cntLoss = 0;
  }
  else if (cntLoss == 2)
  {
    MqttCustomPublish(topicRSPSignal, "2");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 2"));
    cntLoss = 0;
  }
  else if (cntLoss <= 1)
  {
    MqttCustomPublish(topicRSPSignal, "3");
    Serial.println(F("CheckPingSend() : MqttCustomPublish: 3"));
    cntLoss = 0;
  }
}