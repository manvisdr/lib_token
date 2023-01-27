// #include <Arduino.h>
// #include <driver/i2s.h>
// #include <math.h>
// #include <arduinoFFT.h>
// #include <timeout.h>
// #include <WiFi.h>
// #include <WiFiClient.h>
// #include <PubSubClient.h>
// #include <Adafruit_MCP23X17.h>

// Adafruit_MCP23X17 mcp;
// WiFiClient client;
// PubSubClient mqtt(client);
// IPAddress mgiServer(203, 194, 112, 238);

// const char ssid[] = "MGI-MNC";
// const char pass[] = "#neurixmnc#";

// #define PIN_SOUND_SCK 13 // BCK // CLOCK
// #define PIN_SOUND_WS 2   // WS
// #define PIN_SOUND_SD 12  // DATA IN
// #define SAMPLES 1024
// const i2s_port_t I2S_PORT = I2S_NUM_1;
// const int BLOCK_SIZE = SAMPLES;
// #define OCTAVES 9

// // our FFT data
// static float real[SAMPLES];
// static float imag[SAMPLES];
// static arduinoFFT fft(real, imag, SAMPLES, SAMPLES);
// static float energy[OCTAVES];

// // A-weighting curve from 31.5 Hz ... 8000 Hz
// static const float aweighting[] = {-39.4, -26.2, -16.1, -8.6, -3.2, 0.0, 1.2, 1.0, -1.1};
// static unsigned int hexingShort = 0;
// static unsigned int hexingLong = 0;
// static unsigned int itronShortFast = 0;
// static unsigned int itronBenar = 0;
// static unsigned int itronGagal = 0;
// static unsigned long ts = millis();
// static unsigned long last = micros();
// unsigned int sum = 0;
// unsigned int mn = 9999;
// unsigned int mx = 0;
// static unsigned int cnts = 0;
// static unsigned long lastTrigger[2] = {0, 0};

// int cnt = 0;
// unsigned int SoundPeak;

// boolean detectShort = false;
// boolean detectLong = false;
// int detectLongCnt;
// int detectShortCnt;
// int cntDetected;
// int detectCntItronGagal;

// Timeout timer;
// Timeout countdown;
// Timeout validasi;
// bool prevAlarmStatus = false;
// bool AlarmStatus = false;
// bool timeAlarm = false;
// float loudness;

// int counter = 0;

// void SoundInit()
// {
//   Serial.println("Configuring I2S...");
//   esp_err_t err;
//   const i2s_config_t i2s_config = {
//       .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX), // Receive, not transfer
//       .sample_rate = 22627,
//       .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
//       .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // for old esp-idf versions use RIGHT
//       // .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S_MSB), // | I2S_COMM_FORMAT_I2S),I2S_COMM_FORMAT_STAND_I2S
//       .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
//       .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Interrupt level 1
//       .dma_buf_count = 8,                       // number of buffers
//       .dma_buf_len = BLOCK_SIZE,                // samples per buffer
//       .use_apll = true};
//   // The pin config as per the setup
//   const i2s_pin_config_t pin_config = {
//       .bck_io_num = PIN_SOUND_SCK, // BCKL
//       .ws_io_num = PIN_SOUND_WS,   // LRCL
//       .data_out_num = -1,          // not used (only for speakers)
//       .data_in_num = PIN_SOUND_SD  // DOUTESP32-INMP441 wiring
//   };
//   // Configuring the I2S driver and pins.
//   // This function must be called before any I2S driver read/write operations.
//   err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
//   if (err != ESP_OK)
//   {
//     Serial.printf("Failed installing driver: %d\n", err);
//     while (true)
//       ;
//   }
//   err = i2s_set_pin(I2S_PORT, &pin_config);
//   if (err != ESP_OK)
//   {
//     Serial.printf("Failed setting pin: %d\n", err);
//     while (true)
//       ;
//   }
//   Serial.println("SoundInit()...Successful!");
// }

// static void integerToFloat(int32_t *integer, float *vReal, float *vImag, uint16_t samples)
// {
//   for (uint16_t i = 0; i < samples; i++)
//   {
//     vReal[i] = (integer[i] >> 16) / 10.0;
//     vImag[i] = 0.0;
//   }
// }
// // calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
// static void calculateEnergy(float *vReal, float *vImag, uint16_t samples)
// {
//   for (uint16_t i = 0; i < samples; i++)
//   {
//     vReal[i] = sq(vReal[i]) + sq(vImag[i]);
//     vImag[i] = 0.0;
//   }
// }
// // sums up energy in bins per octave
// static void sumEnergy(const float *bins, float *energies, int bin_size, int num_octaves)
// {
//   // skip the first bin
//   int bin = bin_size;
//   for (int octave = 0; octave < num_octaves; octave++)
//   {
//     float sum = 0.0;
//     for (int i = 0; i < bin_size; i++)
//     {
//       sum += real[bin++];
//     }
//     energies[octave] = sum;
//     bin_size *= 2;
//   }
// }
// static float decibel(float v)
// {
//   return 10.0 * log(v) / log(10);
// }
// // converts energy to logaritmic, returns A-weighted sum
// static float calculateLoudness(float *energies, const float *weights, int num_octaves, float scale)
// {
//   float sum = 0.0;
//   for (int i = 0; i < num_octaves; i++)
//   {
//     float energy = scale * energies[i];
//     sum += energy * pow(10, weights[i] / 10.0);
//     energies[i] = decibel(energy);
//   }
//   return decibel(sum);
// }

// unsigned int countSetBits(unsigned int n)
// {
//   unsigned int count = 0;
//   while (n)
//   {
//     count += n & 1;
//     n >>= 1;
//   }
//   return count;
// }
// // detecting 2 frequencies. Set wide to true to match the previous and next bin as well
// bool detectFrequency(unsigned int *mem, unsigned int minMatch, double peak, unsigned int bin1, unsigned int bin2, bool wide)
// {
//   *mem = *mem << 1;

//   if (peak == bin1 || peak == bin2 || (wide && (peak == bin1 + 1 || peak == bin1 - 1 || peak == bin2 + 1 || peak == bin2 - 1)))
//   {
//     *mem |= 1;
//   }
//   if (countSetBits(*mem) >= minMatch)
//   {
//     return true;
//   }
//   return false;
// }

// void calculateMetrics(int val)
// {
//   cnt++;
//   sum += val;
//   if (val > mx)
//   {
//     mx = val;
//   }
//   if (val < mn)
//   {
//     mn = val;
//   }
//   // Serial.printf("NOISE >> SUM: %d val:%d MIN:%d MAX:%d AVG:%2.f\n", sum, val, mn, mx, sum / cnt);
// }

// void SoundLooping()
// {
//   static int32_t samples[BLOCK_SIZE];
//   // Read multiple samples at once and calculate the sound pressure
//   size_t num_bytes_read;
//   esp_err_t err = i2s_read(I2S_PORT,
//                            (char *)samples,
//                            BLOCK_SIZE, // the doc says bytes, but its elements.
//                            &num_bytes_read,
//                            portMAX_DELAY); // no timeout
//   int samples_read = num_bytes_read / 8;
//   // integer to float
//   integerToFloat(samples, real, imag, SAMPLES);
//   // apply flat top window, optimal for energy calculations
//   fft.Windowing(FFT_WIN_TYP_FLT_TOP, FFT_FORWARD);
//   fft.Compute(FFT_FORWARD);
//   // calculate energy in each bin
//   calculateEnergy(real, imag, SAMPLES);
//   // sum up energy in bin for each octave
//   sumEnergy(real, energy, 1, OCTAVES);
//   // calculate loudness per octave + A weighted loudness
//   loudness = calculateLoudness(energy, aweighting, OCTAVES, 1.0);
//   // unsigned int peak = (int)floor(fft.MajorPeak());

//   SoundPeak = (int)floor(fft.MajorPeak());
//   // detectLong = detectFrequency(&bell, 22, peak, 117, 243, true);
//   // detectShort = detectFrequency(&bell, 2, peak, 117, 243, true);
//   calculateMetrics(loudness);
//   // Serial.printf("%d %2.f\n", SoundPeak, loudness); //, detectLongCnt, detectShortCnt);
// }

// bool SoundDetectLong()
// {
//   SoundLooping();
//   bool detectedLong = detectFrequency(&hexingLong, 22, SoundPeak, 117, 243, true);
//   bool detectedShort = detectFrequency(&hexingShort, 2, SoundPeak, 117, 243, true);
//   // bool detectedShortItronFast = detectFrequency(&itronShortFast, 6, SoundPeak, 168, 199, true);

//   // Serial.printf("%d %d %d\n", SoundPeak, detectedLong, detectedShort); //, detectLongCnt, detectShortCnt);
//   if (!detectedShort and (detectShortCnt > 35) or detectedLong)
//   {
//     Serial.println("DETECTED LONG BEEP BEEP");
//     cntDetected = 0;
//     detectShortCnt = 0;
//     detectLongCnt = 0;
//     return true;
//   }
//   else
//   {
//     if (detectedLong)
//     {
//       cntDetected++;
//       detectLongCnt++;
//     }
//     // Serial.println("DETEK LONG BEEP");
//     else if (detectedShort) // and !detectLong)
//     {
//       cntDetected++;
//       detectShortCnt++;
//     }
//     else
//     {
//       cntDetected = 0;
//       detectShortCnt = 0;
//       detectLongCnt = 0;
//     }
//     return false;
//   }
// }

// bool SoundDetectShort()
// {
//   SoundLooping();
//   bool detectedLong = detectFrequency(&hexingLong, 22, SoundPeak, 117, 243, true);
//   bool detectedShort = detectFrequency(&hexingShort, 2, SoundPeak, 117, 243, true);
//   bool detectedShortItronFast = detectFrequency(&itronShortFast, 6, SoundPeak, 168, 199, true);

//   // Serial.printf("%d %d %d %d %d\n", SoundPeak, detectedLong, detectedShort, detectLongCnt, detectShortCnt);
//   if (!detectedShort and (detectShortCnt > 1 and detectShortCnt < 35))
//   {
//     Serial.println("DETECTED SHORT BEEP");
//     cntDetected = 0;
//     detectShortCnt = 0;
//     detectLongCnt = 0;
//     return true;
//   }
//   else
//   {
//     if (detectedLong)
//     {
//       cntDetected++;
//       detectLongCnt++;
//     }
//     // Serial.println("DETEK LONG BEEP");
//     else if (detectedShort) // and !detectLong)
//     {
//       cntDetected++;
//       detectShortCnt++;
//     }
//     else
//     {
//       cntDetected = 0;
//       detectShortCnt = 0;
//       detectLongCnt = 0;
//     }
//     return false;
//   }
// }

// bool SoundDetectShortItron()
// {
//   SoundLooping();
//   bool detectedLong = detectFrequency(&hexingLong, 22, SoundPeak, 117, 243, true);
//   // bool detectedShort = detectFrequency(&hexingShort, 2, SoundPeak, 117, 243, true);
//   bool detectedShortItronFast = detectFrequency(&itronShortFast, 6, SoundPeak, 1, 199, true);

//   Serial.printf("%d %d %d %d %d\n", SoundPeak, detectedLong, detectedShortItronFast, detectLongCnt, detectShortCnt);
//   if (detectedShortItronFast and (detectShortCnt > 100 /*and detectShortCnt < 35)*/))
//   {
//     Serial.println("DETECTED SHORT BEEP");
//     cntDetected = 0;
//     detectShortCnt = 0;
//     detectLongCnt = 0;
//     return true;
//   }
//   else
//   {
//     // if (detectedLong)
//     // {
//     //   cntDetected++;
//     //   detectLongCnt++;
//     // }
//     // Serial.println("DETEK LONG BEEP");
//     if (detectedShortItronFast) // and !detectLong)
//     {
//       cntDetected++;
//       detectShortCnt++;
//     }
//     else
//     {
//       cntDetected = 0;
//       detectShortCnt = 0;
//       detectLongCnt = 0;
//     }
//     return false;
//   }
// }

// bool SoundDetectAlarm()
// {
//   SoundLooping();
//   // bool detectedLong = detectFrequency(&hexingLong, 22, SoundPeak, 117, 243, true);
//   // bool detectedShort = detectFrequency(&hexingShort, 2, SoundPeak, 117, 243, true);
//   bool detectedAlarm = detectFrequency(&hexingLong, 2, SoundPeak, 116, 199, true);

//   // Serial.printf("%d %d %2.f %d %d\n", SoundPeak, detectedAlarm, loudness, detectLongCnt, detectShortCnt);
//   if (detectedAlarm and (detectShortCnt > 20 /*and detectShortCnt < 35)*/))
//   {
//     Serial.println("DETECTED SHORT BEEP");
//     cntDetected = 0;
//     detectShortCnt = 0;
//     detectLongCnt = 0;
//     return true;
//   }
//   else
//   {
//     // if (detectedLong)
//     // {
//     //   cntDetected++;
//     //   detectLongCnt++;
//     // }
//     // Serial.println("DETEK LONG BEEP");
//     if (detectedAlarm and loudness > 43) // and !detectLong)
//     {
//       cntDetected++;
//       detectShortCnt++;
//     }
//     else
//     {
//       cntDetected = 0;
//       detectShortCnt = 0;
//       detectLongCnt = 0;
//     }
//     return false;
//   }
// }

// int SoundDetectItronGagal()
// {
//   int returnValue;
//   // if (/*detectedGagal and (*/ detectCntItronGagal > 61) //)
//   // {
//   //   Serial.println("DETECK BENAR");
//   //   cntDetected = 0;
//   //   detectCntItronGagal = 0;
//   //   return 3;
//   // }
//   // else if (detectCntItronGagal != 0 and detectCntItronGagal < 60)
//   // {
//   //   Serial.println("DETECK GAGAL");
//   //   cntDetected = 0;
//   //   detectCntItronGagal = 0;
//   //   return 2;
//   // }
//   // else
//   // {
//   long startTimer = millis();
//   // Serial.println(startTimer + 500);
//   while (millis() < startTimer + 500)
//   {
//     SoundLooping();
//     bool detectedGagal = detectFrequency(&itronGagal, 2, SoundPeak, 186, 196, true);
//     // Serial.printf("%d %d %d\n", SoundPeak, detectedGagal, cntDetected, detectCntItronGagal);
//     if (detectedGagal)
//     {
//       cntDetected++;
//       // Serial.println(cntDetected);
//     }
//     else
//     {
//       detectCntItronGagal = cntDetected;
//       // Serial.println(detectCntItronGagal);
//       cntDetected = 0;
//       // detectCntItronGagal = 0;
//       // Serial.println("DETECK INVALID");
//       // break;
//     }
//     returnValue = detectCntItronGagal;
//   }
//   Serial.println("Timeout");
//   return cntDetected;
// }

// void OutputDeteksi(int val)
// {
//   if (val > 41)
//     Serial.println("DETECK GAGAL");
//   else if (val != 0 && val < 40)
//     Serial.println("DETECK BENAR");
//   else
//     Serial.println("DETECK INVALID");
// }

// bool SoundDetectItronBenar()
// {
//   SoundLooping();
//   int detectCnt;
//   bool detectedBenar = detectFrequency(&itronBenar, 2, SoundPeak, 182, 196, true);
//   // Serial.printf("%d %d %d\n", SoundPeak, detectedBenar, detectCnt);
//   if (detectedBenar and (detectCnt > 3 /*and detectShortCnt < 35)*/))
//   {
//     Serial.println("DETECK BENAR");
//     cntDetected = 0;
//     detectCnt = 0;
//     detectLongCnt = 0;
//     return true;
//   }
//   else
//   {
//     // if (detectedLong)
//     // {
//     //   cntDetected++;
//     //   detectLongCnt++;
//     // }
//     // Serial.println("DETEK LONG BEEP");
//     if (detectedBenar) // and !detectLong)
//     {
//       cntDetected++;
//       detectCnt++;
//     }
//     else
//     {
//       cntDetected = 0;
//       detectCnt = 0;
//     }
//     return false;
//   }
// }

// bool DetectionLowToken()
// {
//   int cntDetectLong;
//   bool timerStart;
//   bool lowToken;
//   long millisNow;
//   long millisDetectLong;
//   long millisNotDetect;
//   if (SoundDetectLong() and !timerStart)
//   {
//     Serial.println("SOUND DETECT LONG");
//     Serial.println("TIME START");
//     millisNow = millis();
//     Serial.println(millisNow);
//     timerStart = true; // so start value is not updated next time around
//     if (millisNow - millisDetectLong >= 1000)
//     {
//       cntDetectLong++;
//       Serial.println(cntDetectLong);
//       if (cntDetectLong > 5)
//       {
//         cntDetectLong = 0;
//         timerStart = false;
//       }
//     }
//   }
//   else
//   {
//     millisNotDetect = millisNow;
//     if (millisNow - millisNotDetect > 5000)
//     {
//       cntDetectLong = 0;
//       timerStart = false;
//       lowToken = false;
//     }
//   }
//   return timerStart;
// }

// void reconnect()
// {
//   // Loop until we're reconnected
//   while (!mqtt.connected())
//   {
//     Serial.print("Attempting MQTT connection...");
//     // Attempt to connect
//     if (mqtt.connect("arduinoClient"))
//     {
//       Serial.println("connected");
//       // Once connected, publish an announcement...
//       mqtt.publish("outTopic", "hello world");
//       // ... and resubscribe
//       mqtt.subscribe("inTopic");
//     }
//     else
//     {
//       Serial.print("failed, rc=");
//       Serial.print(mqtt.state());
//       Serial.println(" try again in 5 seconds");
//       // Wait 5 seconds before retrying
//       delay(5000);
//     }
//   }
// }

// void setup()
// {
//   Serial.begin(115200);
//   SoundInit();
//   timer.prepare(5000);
//   validasi.prepare(3000);
//   countdown.prepare(10000);
// }

// void loop()
// {

//   // SoundLooping();
//   // Serial.println(SoundDetectItronGagal());
//   // OutputDeteksi(SoundDetectItronGagal());
//   // SoundDetectItronBenar();

//   // SoundDetectShortItron();
//   // SoundLooping();

//   // DetectionLowToken();
//   if (SoundDetectAlarm())
//   {
//     timer.start();
//     prevAlarmStatus = true;
//     counter++;
//     Serial.printf("counter = %d\n", counter);
//   }
//   else if (timer.time_over())
//   {
//     // Serial.print("TIMEOUT...");
//     // Serial.println("RESET");
//     AlarmStatus = false;
//     counter = 0;
//   }
//   if (counter > 13)
//   {
//     AlarmStatus = true;
//   }

//   if ((AlarmStatus and timeAlarm) or (prevAlarmStatus and timeAlarm))
//   {
//     // mqtt.publish("/sound", "DETECT ITRON");
//     Serial.println("DETECK LOW TOKEN");
//     Serial.println("SEND NOTIFICATION");
//     counter = 0;
//   }

//   // Serial.println(counter);
// }

///////////////////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <math.h>
#include "sos-iir-filter.h"

//
// Configuration
//

#define LEQ_PERIOD 0.1        // second(s)
#define WEIGHTING A_weighting // Also avaliable: 'C_weighting' or 'None' (Z_weighting)
#define LEQ_UNITS "LAeq"      // customize based on above weighting used
#define DB_UNITS "dBA"        // customize based on above weighting used
#define USE_DISPLAY 1

// NOTE: Some microphones require at least DC-Blocker filter
#define MIC_EQUALIZER INMP441 // See below for defined IIR filters or set to 'None' to disable
#define MIC_OFFSET_DB 3.0103  // Default offset (sine-wave RMS vs. dBFS). Modify this value for linear calibration

// Customize these values from microphone datasheet
#define MIC_SENSITIVITY -26   // dBFS value expected at MIC_REF_DB (Sensitivity value from datasheet)
#define MIC_REF_DB 94.0       // Value at which point sensitivity is specified in datasheet (dB)
#define MIC_OVERLOAD_DB 116.0 // dB - Acoustic overload point
#define MIC_NOISE_DB 29       // dB - Noise floor
#define MIC_BITS 24           // valid number of bits in I2S data
#define MIC_CONVERT(s) (s >> (SAMPLE_BITS - MIC_BITS))
#define MIC_TIMING_SHIFT 0 // Set to one to fix MSB timing for some microphones, i.e. SPH0645LM4H-x

// Calculate reference amplitude value at compile time
constexpr double MIC_REF_AMPL = pow(10, double(MIC_SENSITIVITY) / 20) * ((1 << (MIC_BITS - 1)) - 1);

#define I2S_WS 2
#define I2S_SCK 13
#define I2S_SD 12
#define PIN_SOUND_SCK 13 // BCK // CLOCK
#define PIN_SOUND_WS 2   // WS
#define PIN_SOUND_SD 12  // DATA IN

// I2S peripheral to use (0 or 1)
#define I2S_PORT I2S_NUM_1

//
// IIR Filters
//

// DC-Blocker filter - removes DC component from I2S data
// See: https://www.dsprelated.com/freebooks/filters/DC_Blocker.html
// a1 = -0.9992 should heavily attenuate frequencies below 10Hz
SOS_IIR_Filter DC_BLOCKER = {
  gain : 1.0,
  sos : {{-1.0, 0.0, +0.9992, 0}}
};

//
// Equalizer IIR filters to flatten microphone frequency response
// See respective .m file for filter design. Fs = 48Khz.
//
// Filters are represented as Second-Order Sections cascade with assumption
// that b0 and a0 are equal to 1.0 and 'gain' is applied at the last step
// B and A coefficients were transformed with GNU Octave:
// [sos, gain] = tf2sos(B, A)
// See: https://www.dsprelated.com/freebooks/filters/Series_Second_Order_Sections.html
// NOTE: SOS matrix 'a1' and 'a2' coefficients are negatives of tf2sos output
//

// TDK/InvenSense INMP441
// Datasheet: https://www.invensense.com/wp-content/uploads/2015/02/INMP441.pdf
// B ~= [1.00198, -1.99085, 0.98892]
// A ~= [1.0, -1.99518, 0.99518]
SOS_IIR_Filter INMP441 = {
  gain : 1.00197834654696,
  sos : {// Second-Order Sections {b1, b2, -a1, -a2}
         {-1.986920458344451, +0.986963226946616, +1.995178510504166, -0.995184322194091}}
};

//
// Weighting filters
//

//
// A-weighting IIR Filter, Fs = 48KHz
// (By Dr. Matt L., Source: https://dsp.stackexchange.com/a/36122)
// B = [0.169994948147430, 0.280415310498794, -1.120574766348363, 0.131562559965936, 0.974153561246036, -0.282740857326553, -0.152810756202003]
// A = [1.0, -2.12979364760736134, 0.42996125885751674, 1.62132698199721426, -0.96669962900852902, 0.00121015844426781, 0.04400300696788968]
SOS_IIR_Filter A_weighting = {
  gain : 0.169994948147430,
  sos : {// Second-Order Sections {b1, b2, -a1, -a2}
         {-2.00026996133106, +1.00027056142719, -1.060868438509278, -0.163987445885926},
         {+4.35912384203144, +3.09120265783884, +1.208419926363593, -0.273166998428332},
         {-0.70930303489759, -0.29071868393580, +1.982242159753048, -0.982298594928989}}
};

//
// C-weighting IIR Filter, Fs = 48KHz
// Designed by invfreqz curve-fitting, see respective .m file
// B = [-0.49164716933714026, 0.14844753846498662, 0.74117815661529129, -0.03281878334039314, -0.29709276192593875, -0.06442545322197900, -0.00364152725482682]
// A = [1.0, -1.0325358998928318, -0.9524000181023488, 0.8936404694728326   0.2256286147169398  -0.1499917107550188, 0.0156718181681081]
SOS_IIR_Filter C_weighting = {
  gain : -0.491647169337140,
  sos : {
      {+1.4604385758204708, +0.5275070373815286, +1.9946144559930252, -0.9946217070140883},
      {+0.2376222404939509, +0.0140411206016894, -1.3396585608422749, -0.4421457807694559},
      {-2.0000000000000000, +1.0000000000000000, +0.3775800047420818, -0.0356365756680430}}
};

//
// Sampling
//
#define SAMPLE_RATE 48000 // Hz, fixed to design of IIR filters
#define SAMPLE_BITS 32    // bits
#define SAMPLE_T int32_t
#define SAMPLES_SHORT (SAMPLE_RATE / 8) // ~125ms
#define SAMPLES_LEQ (SAMPLE_RATE * LEQ_PERIOD)
#define DMA_BANK_SIZE (SAMPLES_SHORT / 16)
#define DMA_BANKS 32

// Data we push to 'samples_queue'
struct sum_queue_t
{
  // Sum of squares of mic samples, after Equalizer filter
  float sum_sqr_SPL;
  // Sum of squares of weighted mic samples
  float sum_sqr_weighted;
  // Debug only, FreeRTOS ticks we spent processing the I2S data
  uint32_t proc_ticks;
};
QueueHandle_t samples_queue;

// our FFT data
#define SAMPLES 1024
const int BLOCK_SIZE = SAMPLES;
#define OCTAVES 9
static float real[SAMPLES_SHORT];
static float imag[SAMPLES_SHORT];
static arduinoFFT fft(real, imag, SAMPLES_SHORT, SAMPLES_SHORT);
static float energy[OCTAVES];

// Static buffer for block of samples
float samples[/* SAMPLES */ SAMPLES_SHORT] __attribute__((aligned(4)));
sum_queue_t q;
uint32_t Leq_samples = 0;
double Leq_sum_sqr = 0;
double Leq_dB = 0;
size_t bytes_read = 0;

static void integerToFloat(float *integer, float *vReal, float *vImag, uint16_t samples)
{
  for (uint16_t i = 0; i < samples; i++)
  {
    vReal[i] = ((int)integer[i] >> 16) / 10.0;
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

void mic_i2s_init()
{
  // Setup I2S to sample mono channel for SAMPLE_RATE * SAMPLE_BITS
  // NOTE: Recent update to Arduino_esp32 (1.0.2 -> 1.0.3)
  //       seems to have swapped ONLY_LEFT and ONLY_RIGHT channels
  const i2s_config_t i2s_config = {
    mode : i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    sample_rate : SAMPLE_RATE,
    bits_per_sample : i2s_bits_per_sample_t(SAMPLE_BITS),
    channel_format : I2S_CHANNEL_FMT_ONLY_LEFT,
    // communication_format : i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    communication_format : i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    intr_alloc_flags : ESP_INTR_FLAG_LEVEL1,
    dma_buf_count : DMA_BANKS,
    dma_buf_len : DMA_BANK_SIZE,
    use_apll : true,
    tx_desc_auto_clear : false,
    fixed_mclk : 0
  };
  // I2S pin mapping
  const i2s_pin_config_t pin_config = {
    bck_io_num : I2S_SCK,
    ws_io_num : I2S_WS,
    data_out_num : -1, // not used
    data_in_num : I2S_SD
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

#if (MIC_TIMING_SHIFT > 0)
  // Undocumented (?!) manipulation of I2S peripheral registers
  // to fix MSB timing issues with some I2S microphones
  REG_SET_BIT(I2S_TIMING_REG(I2S_PORT), BIT(9));
  REG_SET_BIT(I2S_CONF_REG(I2S_PORT), I2S_RX_MSB_SHIFT);
#endif

  i2s_set_pin(I2S_PORT, &pin_config);
}

#define I2S_TASK_PRI 4
#define I2S_TASK_STACK 2048
//

void mic_looping_tast()
{
}
void mic_i2s_reader_task(void *parameter)
{
  mic_i2s_init();

  // Discard first block, microphone may have startup time (i.e. INMP441 up to 83ms)
  size_t bytes_read = 0;
  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);

  while (true)
  {
    // Block and wait for microphone values from I2S
    //
    // Data is moved from DMA buffers to our 'samples' buffer by the driver ISR
    // and when there is requested ammount of data, task is unblocked
    //
    // Note: i2s_read does not care it is writing in float[] buffer, it will write
    //       integer values to the given address, as received from the hardware peripheral.
    i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(SAMPLE_T), &bytes_read, portMAX_DELAY);

    TickType_t start_tick = xTaskGetTickCount();

    // Convert (including shifting) integer microphone values to floats,
    // using the same buffer (assumed sample size is same as size of float),
    // to save a bit of memory
    SAMPLE_T *int_samples = (SAMPLE_T *)&samples;
    for (int i = 0; i < SAMPLES_SHORT; i++)
      samples[i] = MIC_CONVERT(int_samples[i]);

    sum_queue_t q;
    // Apply equalization and calculate Z-weighted sum of squares,
    // writes filtered samples back to the same buffer.
    q.sum_sqr_SPL = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);

    // Apply weighting and calucate weigthed sum of squares
    q.sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);

    // Debug only. Ticks we spent filtering and summing block of I2S data
    q.proc_ticks = xTaskGetTickCount() - start_tick;

    // Send the sums to FreeRTOS queue where main task will pick them up
    // and further calcualte decibel values (division, logarithms, etc...)
    xQueueSend(samples_queue, &q, portMAX_DELAY);
  }
}

//
// Setup and main loop
//
// Note: Use doubles, not floats, here unless you want to pin
//       the task to whichever core it happens to run on at the moment
//

void setup()
{

  // If needed, now you can actually lower the CPU frquency,
  // i.e. if you want to (slightly) reduce ESP32 power consumption
  setCpuFrequencyMhz(80); // It should run as low as 80MHz

  Serial.begin(112500);
  delay(1000); // Safety
  mic_i2s_init();
  // Create FreeRTOS queue
  // samples_queue = xQueueCreate(8, sizeof(sum_queue_t));

  // Create the I2S reader FreeRTOS task
  // NOTE: Current version of ESP-IDF will pin the task
  //       automatically to the first core it happens to run on
  //       (due to using the hardware FPU instructions).
  //       For manual control see: xTaskCreatePinnedToCore
  // xTaskCreate(mic_i2s_reader_task, "Mic I2S Reader", I2S_TASK_STACK, NULL, I2S_TASK_PRI, NULL);

  // sum_queue_t q;
  // uint32_t Leq_samples = 0;
  // double Leq_sum_sqr = 0;
  // double Leq_dB = 0;

  // Read sum of samaples, calculated by 'i2s_reader_task'
  // while (xQueueReceive(samples_queue, &q, portMAX_DELAY))
  // {

  //   // Calculate dB values relative to MIC_REF_AMPL and adjust for microphone reference
  //   double short_RMS = sqrt(double(q.sum_sqr_SPL) / SAMPLES_SHORT);
  //   double short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(short_RMS / MIC_REF_AMPL);

  //   // In case of acoustic overload or below noise floor measurement, report infinty Leq value
  //   if (short_SPL_dB > MIC_OVERLOAD_DB)
  //   {
  //     Leq_sum_sqr = INFINITY;
  //   }
  //   else if (isnan(short_SPL_dB) || (short_SPL_dB < MIC_NOISE_DB))
  //   {
  //     Leq_sum_sqr = -INFINITY;
  //   }

  //   // Accumulate Leq sum
  //   Leq_sum_sqr += q.sum_sqr_weighted;
  //   Leq_samples += SAMPLES_SHORT;

  //   // When we gather enough samples, calculate new Leq value
  //   if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD)
  //   {
  //     double Leq_RMS = sqrt(Leq_sum_sqr / Leq_samples);
  //     Leq_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Leq_RMS / MIC_REF_AMPL);
  //     Leq_sum_sqr = 0;
  //     Leq_samples = 0;

  //     // Serial output, customize (or remove) as needed
  //     Serial.printf("%.1f\n", Leq_dB);

  //     // Debug only
  //     // Serial.printf("%u processing ticks\n", q.proc_ticks);
  //   }
  // }

  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(int32_t), &bytes_read, portMAX_DELAY);
}

void loop()
{

  i2s_read(I2S_PORT, &samples, SAMPLES_SHORT * sizeof(SAMPLE_T), &bytes_read, portMAX_DELAY);

  SAMPLE_T *int_samples = (SAMPLE_T *)&samples;
  for (int i = 0; i < SAMPLES_SHORT; i++)
    samples[i] = MIC_CONVERT(int_samples[i]);

  sum_queue_t q;
  // Apply equalization and calculate Z-weighted sum of squares,
  // writes filtered samples back to the same buffer.
  q.sum_sqr_SPL = MIC_EQUALIZER.filter(samples, samples, SAMPLES_SHORT);

  // Apply weighting and calucate weigthed sum of squares
  q.sum_sqr_weighted = WEIGHTING.filter(samples, samples, SAMPLES_SHORT);

  int samples_read = bytes_read / 8;

  // integerToFloat(samples, real, imag, SAMPLES_SHORT);

  // fft.Windowing(FFT_WIN_TYP_FLT_TOP, FFT_FORWARD);
  // fft.Compute(FFT_FORWARD);

  // calculateEnergy(real, imag, SAMPLES_SHORT);

  // sumEnergy(real, energy, 1, OCTAVES);

  // unsigned int peak = (int)floor(fft.MajorPeak());

  // Serial.printf("%d\n", peak); //, detectLongCnt, detectShortCnt);
  double short_RMS = sqrt(double(q.sum_sqr_SPL) / SAMPLES_SHORT);
  double short_SPL_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(short_RMS / MIC_REF_AMPL);

  // In case of acoustic overload or below noise floor measurement, report infinty Leq value
  if (short_SPL_dB > MIC_OVERLOAD_DB)
  {
    Leq_sum_sqr = INFINITY;
  }
  else if (isnan(short_SPL_dB) || (short_SPL_dB < MIC_NOISE_DB))
  {
    Leq_sum_sqr = -INFINITY;
  }

  // Accumulate Leq sum
  Leq_sum_sqr += q.sum_sqr_weighted;
  Leq_samples += SAMPLES_SHORT;

  // When we gather enough samples, calculate new Leq value
  if (Leq_samples >= SAMPLE_RATE * LEQ_PERIOD)
  {
    double Leq_RMS = sqrt(Leq_sum_sqr / Leq_samples);
    Leq_dB = MIC_OFFSET_DB + MIC_REF_DB + 20 * log10(Leq_RMS / MIC_REF_AMPL);
    Leq_sum_sqr = 0;
    Leq_samples = 0;

    // Serial output, customize (or remove) as needed
    Serial.printf("%.1f\n", Leq_dB);
    // Serial.printf("%d %.1f\n", peak, Leq_dB);
  }
}