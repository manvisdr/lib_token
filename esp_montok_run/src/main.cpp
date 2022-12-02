#include <Arduino.h>
#include <SettingsManager.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include <PubSubClient.h>
#include <Wire.h>
#include "esp_camera.h"
#include "EEPROM.h"

#include <Adafruit_MCP23017.h>
#include <driver/i2s.h>
#include <math.h>
#include <arduinoFFT.h>
#include <ESP32Ping.h>

#include <TimeLib.h>
#include <TimeAlarms.h>

AlarmId id;

#define EEPROM_SIZE 128

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define PIN_MCP_SCL 12
#define PIN_MCP_SDA 13
#define PIN_SOUND_SCK 14 // BCK // CLOCK 14
#define PIN_SOUND_WS 2   // WS 2
#define PIN_SOUND_SD 15  // DATA IN 15

SettingsManager settings;
BluetoothSerial SerialBT;
WiFiClient wificlient;
PubSubClient mqtt(wificlient);
IPAddress mqttServer(203, 194, 112, 238);

const char *configFile = "/config.json";
char productName[16] = "TOKEN-ONLINE_";
char *wifissidbuff;
char *wifipassbuff;
const char *wifissid = "MGI-MNC";
const char *wifipass = "#neurixmnc#";
char idDevice[9];
char bluetooth_name[sizeof(productName) + sizeof(idDevice)];
const char *mqttChannel = "monTok";
const char *MqttUser = "das";
const char *MqttPass = "mgi2022";

char topicIdSubs[50];
char topicTokenSubs[30];
char topicStatusSubs[30];
char topicPelanganSubs[30];
char PrevToken[21];
char NowToken[21];

long start_wifi_millis;
long wifi_timeout = 10000;
int cntDetected;

unsigned int SoundPeak;

Adafruit_MCP23017 mcp;

#define SAMPLES 1024
const i2s_port_t I2S_PORT = I2S_NUM_1;
const int BLOCK_SIZE = SAMPLES;
#define OCTAVES 9
#define SOUND_TIMEOUT 3000

// our FFT data
static float real[SAMPLES];
static float imag[SAMPLES];
static arduinoFFT fft(real, imag, SAMPLES, SAMPLES);
static float energy[OCTAVES];

// A-weighting curve from 31.5 Hz ... 8000 Hz
static const float aweighting[] = {-39.4, -26.2, -16.1, -8.6, -3.2, 0.0, 1.2, 1.0, -1.1};
static unsigned int bell = 0;

static unsigned long ts = millis();
static unsigned long last = micros();
static unsigned int sum = 0;
static unsigned int mn = 9999;
static unsigned int mx = 0;
static unsigned int cnts = 0;
static unsigned long lastTrigger[2] = {0, 0};

boolean detectShort = false;
boolean detectLong = false;
int detectLongCnt;
int detectShortCnt;

int cnt = 0;

enum ENUM_KONEKSI
{
  NONE,
  BT_INIT,
  WIFI_INIT,
  WIFI_OK,
  WIFI_FAILED
};

enum ENUM_KONEKSI KoneksiStage = NONE;

char received_topic[128];
char received_payload[128];
unsigned int received_length;
bool received_msg = false;

const String serverName = "203.194.112.238";       // REPLACE WITH YOUR Raspberry Pi IP ADDRESS OR DOMAIN NAME
const String serverEndpoint = "/topup/response/?"; // Needs to match upload server endpoint
const String keyName = "\"file\"";                 // Needs to match upload server keyName
const int serverPort = 3300;

// send photo char-status->berhasil ,char-pelanggan->01 ,char-sn->222 ,char-token->12313221414355355552
// return json body
String sendPhoto(char *status, char *pelanggan, char *sn, char *token)
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

  Serial.println("Connecting to server: " + serverName);

  if (wificlient.connect(serverName.c_str(), serverPort))
  {
    Serial.println("Connection successful!");
    String head = "--MGI\r\nContent-Disposition: form-data; name=\"file\"; filename=\"file\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--MGI--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    wificlient.println("POST " + serverEndpoint + "status=" + status + "&pelanggan=" + pelanggan + "&sn=" + sn + "&token=" + token + " HTTP/1.1");
    Serial.println("POST " + serverEndpoint + "status=" + status + "&pelanggan=" + pelanggan + "&sn=" + sn + "&token=" + token + "HTTP/1.1");

    wificlient.println("Host: " + serverName);
    wificlient.println("Content-Length: " + String(totalLen));
    wificlient.println("Content-Type: multipart/form-data; boundary=MGI");
    wificlient.println();
    wificlient.print(head);

    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024)
    {
      if (n + 1024 < fbLen)
      {
        wificlient.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0)
      {
        size_t remainder = fbLen % 1024;
        wificlient.write(fbBuf, remainder);
      }
    }
    wificlient.print(tail);

    esp_camera_fb_return(fb);

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;

    while ((startTimer + timoutTimer) > millis())
    {
      Serial.print(".");
      delay(100);
      while (wificlient.available())
      {
        char c = wificlient.read();
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
    Serial.println();
    wificlient.stop();
    Serial.println(getBody);
  }
  else
  {
    getBody = "Connection to " + serverName + " failed.";
    Serial.println(getBody);
  }
  return getBody;
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
    strcpy(serialBTBuffer, SerialBT.readString().c_str());
    if (WiFIQRCheck(serialBTBuffer))
    {
      WiFiQRParsing(serialBTBuffer, &wifissidbuff, &wifipassbuff);
      Serial.println(wifissidbuff);
      Serial.println(wifipassbuff);
    }
    else
      Serial.println("NOT WIFI QR");
  }
}

void printDeviceAddress()
{
  const uint8_t *point = esp_bt_dev_get_address();
  for (int i = 0; i < 6; i++)
  {
    char str[3];
    sprintf(str, "%02X", (int)point[i]);
    Serial.print(str);

    if (i < 5)
    {
      Serial.print(":");
    }
  }
}

void KoneksiInit()
{
  SerialBT.begin(bluetooth_name);
  SerialBT.register_callback(BluetoothCallback);
  printDeviceAddress();
}

bool SettingsInit()
{
  int response;
  settings.readSettings(configFile);
  sprintf(idDevice, "%d", settings.getInt("device.id"));
  if (strlen(settings.getChar("bluetooth.name")) == 0)
  {
    response = settings.setChar("bluetooth.name", strcat(productName, idDevice));
    settings.writeSettings("/config.json");
    ESP.restart();
  }
  else
    strcpy(bluetooth_name, settings.getChar("bluetooth.name"));
  Serial.println(idDevice);
  Serial.println(bluetooth_name);

  return true;
}

bool SettingsLoad();
bool SettignsSave()
{
  int ip = settings.setInt("lan.ip", 12345678);
  Serial.print("WriteResult:");
  Serial.println(ip);
  settings.writeSettings("/config.json");
  return false;
}

int CheckPingGateway()
{
  int cntLoss;
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

int CheckPingServer()
{
  int cntLoss;
  int pingReturn;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Ping.ping(mqttServer))
      cntLoss = 0;
    else
      cntLoss++;
  }
  else
    pingReturn = 100;
  if (cntLoss > 7)
    pingReturn = 0;
  else if (cntLoss = 0)
    pingReturn = 100;
  else
    pingReturn = 50;
  return cntLoss;
}

int CheckPingGoogle()
{
  int cntLoss;
  int pingReturn;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Ping.ping("www.google.com"))
      cntLoss = 0;
    else
      cntLoss++;
  }
  else
    pingReturn = 100;
  if (cntLoss > 7)
    pingReturn = 0;
  else if (cntLoss = 0)
    pingReturn = 100;
  else
    pingReturn = 50;
  return cntLoss;
}

void CameraInit()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA; // 1600x1200
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  else
    Serial.printf("CameraInit()...Succesfull\n", err);
}

bool CheckTimeout(unsigned long *timeNow, unsigned long *timeStart)
{
  if (*timeNow - *timeStart >= SOUND_TIMEOUT)
    return true;
  else
    return false;

} // CheckTimeout

void SoundInit()
{
  Serial.println("Configuring I2S...");
  esp_err_t err;
  const i2s_config_t i2s_config = {
      .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX), // Receive, not transfer
      .sample_rate = 22627,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // for old esp-idf versions use RIGHT
      // .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S_MSB), // | I2S_COMM_FORMAT_I2S),I2S_COMM_FORMAT_STAND_I2S
      .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Interrupt level 1
      .dma_buf_count = 8,                       // number of buffers
      .dma_buf_len = BLOCK_SIZE,                // samples per buffer
      .use_apll = true};
  // The pin config as per the setup
  const i2s_pin_config_t pin_config = {
      .bck_io_num = PIN_SOUND_SCK, // BCKL
      .ws_io_num = PIN_SOUND_WS,   // LRCL
      .data_out_num = -1,          // not used (only for speakers)
      .data_in_num = PIN_SOUND_SD  // DOUTESP32-INMP441 wiring
  };
  // Configuring the I2S driver and pins.
  // This function must be called before any I2S driver read/write operations.
  err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK)
  {
    Serial.printf("Failed installing driver: %d\n", err);
    while (true)
      ;
  }
  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK)
  {
    Serial.printf("Failed setting pin: %d\n", err);
    while (true)
      ;
  }
  Serial.println("I2S driver installed.");
}

static void integerToFloat(int32_t *integer, float *vReal, float *vImag, uint16_t samples)
{
  for (uint16_t i = 0; i < samples; i++)
  {
    vReal[i] = (integer[i] >> 16) / 10.0;
    vImag[i] = 0.0;
  }
}
// calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
static void calculateEnergy(float *vReal, float *vImag, uint16_t samples)
{
  for (uint16_t i = 0; i < samples; i++)
  {
    vReal[i] = sq(vReal[i]) + sq(vImag[i]);
    vImag[i] = 0.0;
  }
}
// sums up energy in bins per octave
static void sumEnergy(const float *bins, float *energies, int bin_size, int num_octaves)
{
  // skip the first bin
  int bin = bin_size;
  for (int octave = 0; octave < num_octaves; octave++)
  {
    float sum = 0.0;
    for (int i = 0; i < bin_size; i++)
    {
      sum += real[bin++];
    }
    energies[octave] = sum;
    bin_size *= 2;
  }
}
static float decibel(float v)
{
  return 10.0 * log(v) / log(10);
}
// converts energy to logaritmic, returns A-weighted sum
static float calculateLoudness(float *energies, const float *weights, int num_octaves, float scale)
{
  float sum = 0.0;
  for (int i = 0; i < num_octaves; i++)
  {
    float energy = scale * energies[i];
    sum += energy * pow(10, weights[i] / 10.0);
    energies[i] = decibel(energy);
  }
  return decibel(sum);
}

unsigned int countSetBits(unsigned int n)
{
  unsigned int count = 0;
  while (n)
  {
    count += n & 1;
    n >>= 1;
  }
  return count;
}
// detecting 2 frequencies. Set wide to true to match the previous and next bin as well
bool detectFrequency(unsigned int *mem, unsigned int minMatch, double peak, unsigned int bin1, unsigned int bin2, bool wide)
{
  *mem = *mem << 1;

  if (peak == bin1 || peak == bin2 || (wide && (peak == bin1 + 1 || peak == bin1 - 1 || peak == bin2 + 1 || peak == bin2 - 1)))
  {
    *mem |= 1;
  }
  if (countSetBits(*mem) >= minMatch)
  {
    return true;
  }
  return false;
}

void calculateMetrics(int val)
{
  cnt++;
  sum += val;
  if (val > mx)
  {
    mx = val;
  }
  if (val < mn)
  {
    mn = val;
  }
}

void SoundLooping()
{
  static int32_t samples[BLOCK_SIZE];
  // Read multiple samples at once and calculate the sound pressure
  size_t num_bytes_read;
  esp_err_t err = i2s_read(I2S_PORT,
                           (char *)samples,
                           BLOCK_SIZE, // the doc says bytes, but its elements.
                           &num_bytes_read,
                           portMAX_DELAY); // no timeout
  int samples_read = num_bytes_read / 8;
  // integer to float
  integerToFloat(samples, real, imag, SAMPLES);
  // apply flat top window, optimal for energy calculations
  fft.Windowing(FFT_WIN_TYP_FLT_TOP, FFT_FORWARD);
  fft.Compute(FFT_FORWARD);
  // calculate energy in each bin
  calculateEnergy(real, imag, SAMPLES);
  // sum up energy in bin for each octave
  sumEnergy(real, energy, 1, OCTAVES);
  // calculate loudness per octave + A weighted loudness
  float loudness = calculateLoudness(energy, aweighting, OCTAVES, 1.0);
  SoundPeak = (int)floor(fft.MajorPeak());

  // Serial.println(peak);
  // Serial.printf("%d %d %d \n", peak, detectLong, detectShort);

  // detectLong = detectFrequency(&bell, 22, peak, 117, 243, true);
  // detectShort = detectFrequency(&bell, 2, peak, 117, 243, true);

  // if (detectLong)
  //   Serial.println("Detect Long");
  // else if (detectShort)
  //   Serial.println("Detect Short");
}

bool SoundDetectLong()
{
  SoundLooping();
  bool detectedLong = detectFrequency(&bell, 22, SoundPeak, 117, 243, true);
  bool detectedShort = detectFrequency(&bell, 2, SoundPeak, 117, 243, true);
  Serial.printf("%d %d %d %d", SoundPeak, detectedLong, detectedShort, cntDetected);
  if (detectedLong or detectedShort)
  {
    cntDetected++;
  }
  if (cntDetected > 35)
  {
    cntDetected = 0;
    return true;
  }
  else
  {
    cntDetected = 0;
    return false;
  }
}

bool SoundDetectShort()
{
  SoundLooping();
  bool detectedLong = detectFrequency(&bell, 22, SoundPeak, 117, 243, true);
  bool detectedShort = detectFrequency(&bell, 2, SoundPeak, 117, 243, true);

  Serial.printf("%d %d %d %d %d\n", SoundPeak, detectedLong, detectedShort, detectLongCnt, detectShortCnt);
  if (!detectedShort and (detectShortCnt > 1 and detectShortCnt < 35))
  {
    Serial.println("DETECTED SHORT BEEP");
    cntDetected = 0;
    detectShortCnt = 0;
    detectLongCnt = 0;
    return true;
  }
  else
  {
    if (detectedLong)
    {
      cntDetected++;
      detectLongCnt++;
    }
    // Serial.println("DETEK LONG BEEP");
    else if (detectedShort) // and !detectLong)
    {
      cntDetected++;
      detectShortCnt++;
    }
    else
    {
      cntDetected = 0;
      detectShortCnt = 0;
      detectLongCnt = 0;
    }
    return false;
  }
}

void MechanicInit()
{
  Wire.begin(PIN_MCP_SDA, PIN_MCP_SCL);
  mcp.begin(&Wire);
  for (int i = 0; i < 16; i++)
  {
    if (i < 8)
      mcp.pinMode(i, INPUT_PULLUP);
    // Serial.printf("INPUT - %d\n", i);
    else
      mcp.pinMode(i, OUTPUT);
    // Serial.printf("OUTPUT - %d\n", i);
  }
}

void WifiInit()
{
  WiFi.begin(wifissid, wifipass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (millis() - start_wifi_millis > wifi_timeout)
    {
      WiFi.disconnect(true, true);
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void callback(char *topic, byte *payload, unsigned int length)
{
  strcpy(received_topic, topic);
  memcpy(received_payload, payload, length);
  received_msg = true;
  received_length = length;

  Serial.printf("received_topic %s received_length = %d \n", received_topic, received_length);
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
    // mqtt.setServer(mqttServer, 1883);
    // mqtt.setCallback(callback);
    Serial.println(String("Attempting MQTT broker:") + mqttServer);

    // for (uint8_t i = 0; i < 8; i++)
    // {
    //   clientId[i] = alphanum[random(62)];
    // }
    // clientId[8] = '\0';
    // Setting mqtt gaes
    if (mqtt.connect(mqttChannel, MqttUser, MqttPass))
    {
      Serial.println("Established:" + String(clientId));
      mqtt.subscribe(topicTokenSubs);
      mqtt.subscribe(topicIdSubs);
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
  sprintf(topicIdSubs, "%s/%s/%s", mqttChannel, idDevice, "id");
  sprintf(topicTokenSubs, "%s/%s/%s", mqttChannel, idDevice, "token");
  sprintf(topicStatusSubs, "%s/%s/%s", mqttChannel, idDevice, "state");
  sprintf(topicPelanganSubs, "%s/%s/%s", mqttChannel, idDevice, "pelanggan");
  // MqttCustomPublish(topicIdSubs, " ");
  // MqttCustomPublish(topicTokenSubs, " ");
  // MqttCustomPublish(topicStatusSubs, " ");
  // MqttCustomPublish(topicPelanganSubs, " ");
}

/* *********************************** MQTT ************************* */

void MqttStatusPublish(String msg)
{
  mqtt.publish(topicStatusSubs, msg.c_str());
}

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
      // Once connected, publish an announcement...
      MqttStatusPublish("connect");
      // ... and resubscribe
      // mqtt.subscribe(topicPelanganSubs);
      mqtt.subscribe(topicStatusSubs);
      mqtt.subscribe(topicTokenSubs);
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

void DetectionLowToken()
{
}

void GetKwhProcess()
{
  if (received_msg and strcmp(received_payload, "getkwh-request") == 0)
  {
    Serial.println("GET_KWH");
    Serial.println(sendPhoto("gwtkwh-success", "01", "222", "12313221414355355552"));
    MqttCustomPublish(topicStatusSubs, "gwtkwh-success");
    received_msg = false;
  }
}

void TokenProcess(/*char *token*/)
{
  // Serial.println(received_msg);
  // Serial.println(received_topic);
  // Serial.println(received_length);
  if (received_msg and (strcmp(received_topic, topicTokenSubs) == 0) and received_length == 20)
  {

    // if (strcmp(token, PrevToken) == 0)
    // {
    //   MqttCustomPublish(topicStatusSubs, "failed");
    // }
    // else
    // {
    for (int i = 0; i < received_length; i++)
    {
      char pyld = received_payload[i];
      unsigned long timeNow = millis();
      unsigned long timeStart;
      Serial.printf("CHAR KE %d data %c - ", i, pyld);
      switch (pyld)
      {
      case '0':
        Serial.printf("SELENOID %c ON\n", pyld);
        timeStart = timeNow;
        while (!SoundDetectShort())
        {
          Serial.println("WAITING RESPONSE FROM SOUND");
          if (CheckTimeout(&timeNow, &timeStart))
          {
            Serial.printf("SELENOID %c OFF\n", pyld);
            break;
          }
        }
      case '1':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      case '2':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      case '3':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      case '4':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      case '5':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      case '6':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      case '7':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      case '8':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      case '9':
        Serial.printf("SELENOID %c ON\n", pyld);
        break;
      default:
        break;
      }
    }
    // }
    Serial.println(sendPhoto("success", "01", "222", "12313221414355355552"));
    MqttCustomPublish(topicStatusSubs, "success");
    received_msg = false;
  }
}

void runalarm4()
{
  Serial.println(sendPhoto("gwtkwh-success", "01", "222", "12313221414355355552"));
}

void setup()
{
  Serial.begin(115200);
  // MechanicInit();
  SettingsInit();
  KoneksiInit();
  CameraInit();
  // SoundInit();
  WifiInit();
  mqtt.setServer(mqttServer, 1883);
  mqtt.setCallback(callback);

  // if (MqttConnect())
  MqttTopicInit();
  MqttReconnect();
  MqttCustomPublish(topicIdSubs, " ");
  MqttCustomPublish(topicTokenSubs, " ");
  MqttCustomPublish(topicStatusSubs, " ");
  MqttCustomPublish(topicPelanganSubs, " ");
  Serial.println(topicIdSubs);
  Serial.println(topicTokenSubs);
  Serial.println(topicStatusSubs);
  Serial.println(topicPelanganSubs);

  id = Alarm.timerRepeat(0, 15, 0, runalarm4);
  // MqttPublish(idDevice);
  // Serial.println(CheckPingGateway());
}

void loop()
{
  // Serial.println(CheckPingGateway());
  // delay(1000);
  Alarm.delay(1000);
  if (!mqtt.connected())
  {
    MqttReconnect();
    // MqttConnect();
  }
  TokenProcess();
  GetKwhProcess();
  // // MqttPublish(kirim)
  mqtt.loop();
}