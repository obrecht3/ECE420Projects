#pragma once

#include <jni.h>
#include <vector>
#include "kiss_fft/kiss_fft.h"
#include "ece420_lib.h"
#include "TextParser.h"
#include "PitchEventHandler.h"

class Tuner {
public:
    Tuner(int _frameSize, int _sampleRate);
    ~Tuner();

    // should execute writeInputSamples THEN detectBufferPeriod THEN processBlock to compute full TD-PSOLA
    void writeInputSamples(float *data);
    int detectBufferPeriod(); // this is separate to eventually cut down on computation for multiple melodies
    void processBlock(float *data, std::vector<PitchEvent> pitchEvents, int periodLen);

private:
    bool pitchShift(std::vector<PitchEvent> pitchEvents, int periodLen);
    void findEpochLocations(std::vector<int> &epochLocations, float *buffer, int periodLen);
    void overlapAddArray(float *dest, float *src, int startIdx, int len);

private:
    int bufferSize;
    int frameSize;
    int sampleRate;
    int newEpochIdx;

    PitchEventHandler eventHandler;

    std::vector<float> bufferIn;
    std::vector<float> bufferOut;

    const int EPOCH_PEAK_REGION_WIGGLE = 30;
    const int VOICED_THRESHOLD = 100000000;
};