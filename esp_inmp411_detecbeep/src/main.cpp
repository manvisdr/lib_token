#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>
#include <arduinoFFT.h>
#include <timeout.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Adafruit_MCP23017.h>
// #include <Adafruit_MCP23X17.h>
#define PIN_MCP_SCL 15
#define PIN_MCP_SDA 14
Adafruit_MCP23017 mcp;
WiFiClient client;
PubSubClient mqtt(client);
IPAddress mgiServer(203, 194, 112, 238);

const char ssid[] = "lepi";
const char pass[] = "1234567890";

#define PIN_SOUND_SCK 13 // BCK // CLOCK
#define PIN_SOUND_WS 2   // WS
#define PIN_SOUND_SD 12  // DATA IN
#define SAMPLES 1024
const i2s_port_t I2S_PORT = I2S_NUM_1;
const int BLOCK_SIZE = SAMPLES;
#define OCTAVES 9

// our FFT data
static float real[SAMPLES];
static float imag[SAMPLES];
static arduinoFFT fft(real, imag, SAMPLES, SAMPLES);
static float energy[OCTAVES];

// A-weighting curve from 31.5 Hz ... 8000 Hz
static const float aweighting[] = {-39.4, -26.2, -16.1, -8.6, -3.2, 0.0, 1.2, 1.0, -1.1};
static unsigned int hexingShort = 0;
static unsigned int hexingLong = 0;
static unsigned int itronShortFast = 0;
static unsigned int itronBenar = 0;
static unsigned int itronGagal = 0;
static unsigned int soundValidasi;
static unsigned long ts = millis();
static unsigned long last = micros();
unsigned int sum = 0;
unsigned int mn = 9999;
unsigned int mx = 0;
static unsigned int cnts = 0;
static unsigned long lastTrigger[2] = {0, 0};

int cnt = 0;
unsigned int SoundPeak;

boolean detectShort = false;
boolean detectLong = false;
int detectLongCnt;
int detectShortCnt;
int cntDetected;
int detectCntItronGagal;

Timeout timer;
Timeout countdown;

bool prevAlarmStatus = false;
bool AlarmStatus = false;
bool timeAlarm = false;
float loudness;

int counter = 0;

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
  Serial.println("SoundInit()...Successful!");
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
  // Serial.printf("NOISE >> SUM: %d val:%d MIN:%d MAX:%d AVG:%2.f\n", sum, val, mn, mx, sum / cnt);
}

void MqttCustomPublish(char *path, String msg)
{
  mqtt.publish(path, msg.c_str());
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
  loudness = calculateLoudness(energy, aweighting, OCTAVES, 1.0);
  // unsigned int peak = (int)floor(fft.MajorPeak());

  SoundPeak = (int)floor(fft.MajorPeak());

  calculateMetrics(loudness);
  // MqttCustomPublish("/sound", String(SoundPeak));
  // Serial.printf("%d %2.f\n", SoundPeak, loudness); //, detectLongCnt, detectShortCnt);
}

bool SoundDetectAlarm()
{
  SoundLooping();
  bool detectedAlarm = detectFrequency(&hexingLong, 6, SoundPeak, 182, 199, true);

  Serial.printf("%d %d %2.f %d\n", SoundPeak, detectedAlarm, loudness, detectedAlarm);
  if (detectedAlarm and (detectShortCnt > 15 /*and detectShortCnt < 35)*/))
  {
    // Serial.println("DETECTED SHORT BEEP");
    cntDetected = 0;
    detectShortCnt = 0;
    detectLongCnt = 0;
    return true;
  }
  else
  {
    if (detectedAlarm and loudness > 20) // and !detectLong)
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

void reconnect()
{
  // Loop until we're reconnected
  while (!mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect("zzzz"))
    {
      Serial.println("connected");
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

void AlarmDetected()
{
  if (SoundDetectAlarm())
  {
    timer.start(6000);
    prevAlarmStatus = true;
    counter++;
    Serial.printf("Frekuensi Detected. Counter %d\n", counter);
  }
  else if (timer.time_over())
  {
    // Serial.print("TIMEOUT...");
    // Serial.println("RESET");
    AlarmStatus = false;
    counter = 0;
  }
  if (counter > 20)
  {
    AlarmStatus = true;
  }

  if (countdown.time_over())
  {
    Serial.println("COUNTDOWN TIME OVER");
    if (counter == 0)
    {
      Serial.println("COUNTDOWN START , COUNTER 0");
      countdown.start(60000);
    }
    else
    {
      Serial.println("COUNTDOWN TIME OVER, TIME ALARM TRUE");
      timeAlarm = true;
    }
  }

  if (AlarmStatus and timeAlarm)
  {
    mqtt.publish("Dummy/sound", "sendAlarm");
    Serial.println("SEND NOTIFICATION");
    AlarmStatus = false;
    prevAlarmStatus = false;
    timeAlarm = false;
    countdown.start(60000);
  }
}
int soundValidasiCnt = 0;
int SoundDetectValidasi()
{
  int validasiCnt;
  SoundLooping();
  bool detectedGagal = detectFrequency(&soundValidasi, 2, SoundPeak, 182, 196, true);

  // MqttCustomPublish(topicRSPSoundPeak, String(SoundPeak));
  // MqttCustomPublish(topicRSPSoundStatusVal, String(soundValidasiCnt));
  if (detectedGagal /* and loudness > 40 */)
  {
    soundValidasiCnt++;
    // Serial.println(soundValidasiCnt);
  }
  Serial.print(SoundPeak);
  Serial.print(" ");
  Serial.println(soundValidasiCnt);
}

void MechanicInit()
{
  Wire.begin(PIN_MCP_SDA, PIN_MCP_SCL);
  mcp.begin(&Wire);
  for (int i = 0; i < 16; i++)
  {
    mcp.pinMode(i, OUTPUT); // MCP23017 pins are set for inputs
  }
  Serial.println("MechanicInit()...Successful");
}

bool MechanicTyping(int pin)
{
  mcp.digitalWrite(pin, HIGH);
  delay(150);
  mcp.digitalWrite(pin, LOW);
  return true;
}

void MechanicEnter()
{
  MechanicTyping(11);
}

int prevVal;
int nowVal;
bool bolprevVal;
bool bolnowVal;
bool bolnexVal;
bool outVal;
int bin1 = 182;
bool peakAwal;
int counterNew;

int valmilis = 10000;
long solenoidMillis;
long afterSolenoidMillis;

int begin_pos;
int last_pos;
int detected_pos;
int detected_pos1;
int valueInterval;
int counterBeep;
float division;
bool flagEnter = false;
int counterAlarm;

void setup()
{
  Serial.begin(115200);
  MechanicInit();
  SoundInit();
  solenoidMillis = 5000;
  // timer.prepare(6000);
  // countdown.prepare(60000);
  // WiFi.begin(ssid, pass);
  // mqtt.setServer(mgiServer, 1883);
  // mqtt.setCallback(callback);
}

void DeteksiAlarm()
{
  SoundLooping();
  bolprevVal = bolnowVal;
  bolnowVal = bolnexVal;
  bolnexVal = SoundPeak == bin1 || (SoundPeak == bin1 + 1 || SoundPeak == bin1 - 1);
  if (bolnowVal > bolnexVal)
  {
    counterAlarm++;
  }

  else if (bolnowVal > bolnexVal)
  {
    counterAlarm++;
  }
  else
    counterNew++;
}

void DeteksiValidasi()
{
  if (millis() > solenoidMillis + valmilis and !flagEnter)
  {
    Serial.println("STATE SOLENOID ENTER");
    MechanicEnter();
    flagEnter = true;
    afterSolenoidMillis = millis();
    Serial.println("STATE RECORD MULAI");
  }

  if (flagEnter and millis() < afterSolenoidMillis + 5000)
  {
    // Serial.printf("millis() =%d RecordValidasiSound()\n", millis());

    SoundLooping();

    bolprevVal = bolnowVal;
    bolnowVal = bolnexVal;
    bolnexVal = SoundPeak == bin1 || (SoundPeak == bin1 + 1 || SoundPeak == bin1 - 1);

    if ((bolprevVal < bolnowVal))
    {
      begin_pos = counterNew;
    }

    else if (bolnowVal > bolnexVal)
    {
      last_pos = counterNew;
      counterBeep++;
      valueInterval += last_pos - begin_pos;
    }
    else
      counterNew++;
    // Serial.print(SoundPeak);
    // Serial.print(" ");
    // Serial.print(detected_pos);
    // Serial.print(" ");
    // Serial.print(abs(detected_pos1));
    // Serial.print(" ");
    // Serial.print(abs(detected_pos1));
    // Serial.print(" ");
    Serial.print(SoundPeak);
    Serial.print(" ");
    Serial.print(begin_pos);
    Serial.print(" ");
    Serial.print(last_pos);
    Serial.print(" ");
    Serial.print(valueInterval);
    Serial.print(" ");
    Serial.println(counterBeep);
  }

  if (flagEnter and millis() > afterSolenoidMillis + 5000)
  {
    Serial.println("STATE RECORD SELESAI");
    division = valueInterval / counterBeep;
    Serial.printf("val:%d counter:%d bagi:%f\n", valueInterval, counterBeep, division);
    last_pos = 0;
    begin_pos = 0;
    counterBeep = 0;
    valueInterval = 0;
    solenoidMillis = millis();
    flagEnter = false;
  }
}

void loop()
{
  // SoundLooping();

  // SoundDetectValidasi();

  // SoundLooping();
  // if(detectedGagal)
  //   soundValidasiCnt++;
  //   else
  //   soundValidasiCnt = 0;
  // Serial.print(SoundPeak);
  // Serial.print(" ");
  // Serial.println(soundValidasiCnt);

  //////===========LOGIC DETEKSI===========//
  // detected_pos = counterNew - last_pos;
  // detected_pos1 = last_pos - begin_pos;

  // bolprevVal = bolnowVal;
  // bolnowVal = bolnexVal;
  // bolnexVal = SoundPeak == bin1 || (SoundPeak == bin1 + 1 || SoundPeak == bin1 - 1);

  // if ((bolprevVal < bolnowVal))
  // {
  //   begin_pos = counterNew;
  // }
  // // else if ((bolprevVal < bolnowVal) and (bolnowVal > bolnexVal))
  // // {
  // //   last_pos = counterNew;
  // // }
  // else if (bolnowVal > bolnexVal)
  // {
  //   last_pos = counterNew;
  //   counterBeep++;
  //   valueInterval += last_pos - begin_pos;
  // }
  // else
  //   counterNew++;
  //////===========LOGIC DETEKSI===========//

  // Serial.print(SoundPeak);
  // Serial.print(" ");
  // Serial.print(detected_pos);
  // Serial.print(" ");
  // Serial.print(abs(detected_pos1));
  // Serial.print(" ");
  // Serial.print(abs(detected_pos1));
  // Serial.print(" ");
  // Serial.println(valueInterval);
}
