#pragma once

#include <jni.h>
#include <vector>
#include "kiss_fft/kiss_fft.h"
#include "ece420_lib.h"

class Tuner {
public:
    Tuner(int _frameSize, int _sampleRate);
    ~Tuner();

    void processBlock(int16_t *data);

private:
    bool pitchShift(float targetFreq);
    int detectBufferPeriod(float *buffer);
    void findEpochLocations(std::vector<int> &epochLocations, float *buffer, int periodLen);
    void overlapAddArray(float *dest, float *src, int startIdx, int len);

private:
    int bufferSize;
    int frameSize;
    int sampleRate;
    int newEpochIdx;

    std::vector<float> bufferIn;
    std::vector<float> bufferOut;

    const int EPOCH_PEAK_REGION_WIGGLE = 30;
    const int VOICED_THRESHOLD = 100000000;
};