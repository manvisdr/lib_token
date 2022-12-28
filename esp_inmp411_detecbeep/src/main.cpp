#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>
#include <arduinoFFT.h>
#include <timeout.h>

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
static unsigned int bell = 0;
static unsigned int fireAlarm = 0;
static unsigned long ts = millis();
static unsigned long last = micros();
static unsigned int sum = 0;
static unsigned int mn = 9999;
static unsigned int mx = 0;
static unsigned int cnts = 0;
static unsigned long lastTrigger[2] = {0, 0};

int cnt = 0;
unsigned int SoundPeak;

boolean detectShort = false;
boolean detectLong = false;
int detectLongCnt;
int detectShortCnt;
int cntDetected;

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
  // unsigned int peak = (int)floor(fft.MajorPeak());

  SoundPeak = (int)floor(fft.MajorPeak());
  // detectLong = detectFrequency(&bell, 22, peak, 117, 243, true);
  // detectShort = detectFrequency(&bell, 2, peak, 117, 243, true);
}

bool SoundDetectLong()
{
  SoundLooping();
  bool detectedLong = detectFrequency(&bell, 22, SoundPeak, 117, 243, true);
  bool detectedShort = detectFrequency(&bell, 2, SoundPeak, 117, 243, true);

  // Serial.printf("%d %d %d\n", SoundPeak, detectedLong, detectedShort); //, detectLongCnt, detectShortCnt);
  if (!detectedShort and (detectShortCnt > 35) or detectedLong)
  {
    Serial.println("DETECTED LONG BEEP BEEP");
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

bool SoundDetectShort()
{
  SoundLooping();
  bool detectedLong = detectFrequency(&bell, 22, SoundPeak, 117, 243, true);
  bool detectedShort = detectFrequency(&bell, 2, SoundPeak, 117, 243, true);

  // Serial.printf("%d %d %d %d %d\n", SoundPeak, detectedLong, detectedShort, detectLongCnt, detectShortCnt);
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

bool DetectionLowToken()
{
  int cntDetectLong;
  bool timerStart;
  bool lowToken;
  long millisNow;
  long millisDetectLong;
  long millisNotDetect;
  if (SoundDetectLong() and !timerStart)
  {
    Serial.println("SOUND DETECT LONG");
    Serial.println("TIME START");
    millisNow = millis();
    Serial.println(millisNow);
    timerStart = true; // so start value is not updated next time around
    if (millisNow - millisDetectLong >= 1000)
    {
      cntDetectLong++;
      Serial.println(cntDetectLong);
      if (cntDetectLong > 5)
      {
        cntDetectLong = 0;
        timerStart = false;
      }
    }
  }
  else
  {
    millisNotDetect = millisNow;
    if (millisNow - millisNotDetect > 5000)
    {
      cntDetectLong = 0;
      timerStart = false;
      lowToken = false;
    }
  }
  return timerStart;
}
Timeout timer;
int counter = 0;
void setup()
{
  Serial.begin(115200);
  timer.prepare(5000);
  SoundInit();
}

void loop()
{
  // DetectionLowToken();
  if (SoundDetectLong())
  {
    timer.start();
    counter++;
  }
  else if (timer.time_over())
  {
    Serial.print("TIMEOUT...");
    Serial.println("RESET");
    counter = 0;
  }

  if (/* timer.time_over()  and */ counter > 13)
  {
    Serial.println("DETECK LOW TOKEN");
    Serial.println("SEND NOTIFICATION");
    counter = 0;
  }
  // else if (timer.time_over() and counter < 13)
  // {
  //   Serial.println("UNDER COUNTER ...");
  //   Serial.println("RESET");
  //   counter = 0;
  // }

  Serial.println(counter);
}