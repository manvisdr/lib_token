#include <Type.h>
void SoundInit();

static void integerToFloat(int32_t *integer, float *vReal, float *vImag, uint16_t samples);

// calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
static void calculateEnergy(float *vReal, float *vImag, uint16_t samples);

// sums up energy in bins per octave
static void sumEnergy(const float *bins, float *energies, int bin_size, int num_octaves);

static float decibel(float v);

// converts energy to logaritmic, returns A-weighted sum
static float calculateLoudness(float *energies, const float *weights, int num_octaves, float scale);

unsigned int countSetBits(unsigned int n);

// detecting 2 frequencies. Set wide to true to match the previous and next bin as well
bool detectFrequency(unsigned int *mem, unsigned int minMatch, double peak, unsigned int bin1, unsigned int bin2, bool wide);

void calculateMetrics(int val);

void SoundLooping();

bool SoundDetectLong();

bool SoundDetectShort();