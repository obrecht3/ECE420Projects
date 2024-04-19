//
// Created by joels on 4/18/2024.
//

#include "Tuner.h"
#include "kiss_fft/kiss_fft.h"
#include <math.h>

Tuner::Tuner(int _frameSize, int _sampleRate)
    : bufferSize{3 * _frameSize}
    , frameSize{_frameSize}
    , sampleRate{_sampleRate}
    , newEpochIdx{frameSize}
    , currPitchEvent{0, 0}
    , bufferIn(bufferSize, 0)
    , bufferOut(bufferSize, 0){
}

Tuner::~Tuner() {

}

void Tuner::processBlock(int16_t *data, std::vector<PitchEvent> pitchEvents) {
    // Shift our old data back to make room for the new data
    for (int i = 0; i < 2 * frameSize; i++) {
        bufferIn[i] = bufferIn[i + frameSize - 1];
    }

    // Finally, put in our new data.
    for (int i = 0; i < frameSize; i++) {
        bufferIn[i + 2 * frameSize - 1] = (float) data[i];
    }

    // The whole kit and kaboodle -- pitch shift
    bool isVoiced = pitchShift(pitchEvents);

    if (isVoiced) {
        for (int i = 0; i < frameSize; i++) {
            data[i] = (int16_t) bufferOut[i];
        }
    }

    // Very last thing, update your output circular buffer!
    for (int i = 0; i < 2 * frameSize; i++) {
        bufferOut[i] = bufferOut[i + frameSize - 1];
    }

    for (int i = 0; i < frameSize; i++) {
        bufferOut[i + 2 * frameSize - 1] = 0;
    }
}

bool Tuner::pitchShift(std::vector<PitchEvent> pitchEvents) {
    // Lab 4 code is condensed into this function
    int periodLen = detectBufferPeriod(bufferIn.data());

    // If voiced
    if (periodLen > 0) {
        std::vector<int> epochLocations;
        findEpochLocations(epochLocations, bufferIn.data(), periodLen);

        int closestIdx = 1;
        const int P0 = periodLen;
        const int l = 2 * P0 + 1;
        float hWindowed[l];

        int pitchEventIdx = 0;
        int P1 = 1;
        if (currPitchEvent.frequency > 0) {
            P1 = static_cast<int>(static_cast<float>(sampleRate) / currPitchEvent.frequency);
        }

        while (newEpochIdx < 2 * frameSize) {
            // Find the closest epoch in the original signal
            closestIdx = findClosestInVector(epochLocations, newEpochIdx, closestIdx - 1, epochLocations.size() - 2);
            const int nextPitchEventIdx = setCurrPitchEvent(pitchEventIdx, newEpochIdx - frameSize, pitchEvents);
            if (nextPitchEventIdx >= 0) {
                pitchEventIdx = nextPitchEventIdx;
                P1 = static_cast<int>(static_cast<float>(sampleRate) / currPitchEvent.frequency);
            }

            // apply Hanning window
            for(int i = 0; i < l; i++) {
                const int pos = (epochLocations[closestIdx] - P0 + i + bufferSize) % (bufferSize);
                hWindowed[i] = getHanningCoef(l,i) * bufferIn[pos];
            }

            // overlap
            overlapAddArray(bufferOut.data(), hWindowed, newEpochIdx - P0,l);
            newEpochIdx += P1;
        }
        // ************************ END YOUR CODE HERE  ***************************** //
    }

    // Final bookkeeping, move your new pointer back, because you'll be
    // shifting everything back now in your circular buffer
    newEpochIdx -= frameSize;
    if (newEpochIdx < frameSize) {
        newEpochIdx = frameSize;
    }

    return (periodLen > 0);
}

int Tuner::detectBufferPeriod(float *buffer) {

    float totalPower = 0;
    for (int i = 0; i < bufferSize; i++) {
        totalPower += buffer[i] * buffer[i];
    }

    if (totalPower < VOICED_THRESHOLD) {
        return -1;
    }

    // FFT is done using Kiss FFT engine. Remember to free(cfg) on completion
    kiss_fft_cfg cfg = kiss_fft_alloc(bufferSize, false, 0, 0);

    kiss_fft_cpx buffer_in[bufferSize];
    kiss_fft_cpx buffer_fft[bufferSize];

    for (int i = 0; i < bufferSize; i++) {
        buffer_in[i].r = bufferIn[i];
        buffer_in[i].i = 0;
    }

    kiss_fft(cfg, buffer_in, buffer_fft);
    free(cfg);


    // Autocorrelation is given by:
    // autoc = ifft(fft(x) * conj(fft(x))
    //
    // Also, (a + jb) (a - jb) = a^2 + b^2
    kiss_fft_cfg cfg_ifft = kiss_fft_alloc(bufferSize, true, 0, 0);

    kiss_fft_cpx multiplied_fft[bufferSize];
    kiss_fft_cpx autoc_kiss[bufferSize];

    for (int i = 0; i < bufferSize; i++) {
        multiplied_fft[i].r = (buffer_fft[i].r * buffer_fft[i].r)
                              + (buffer_fft[i].i * buffer_fft[i].i);
        multiplied_fft[i].i = 0;
    }

    kiss_fft(cfg_ifft, multiplied_fft, autoc_kiss);
    free(cfg_ifft);

    // Move to a normal float array rather than a struct array of r/i components
    float autoc[bufferSize];
    for (int i = 0; i < bufferSize; i++) {
        autoc[i] = autoc_kiss[i].r;
    }

    // We're only interested in pitches below 1000Hz.
    // Why does this line guarantee we only identify pitches below 1000Hz?
    int minIdx = sampleRate / 1000;
    int maxIdx = bufferSize / 2;

    int periodLen = findMaxArrayIdx(autoc, minIdx, maxIdx);
    float freq = ((float) sampleRate) / periodLen;

    // TODO: tune (set freq < 1)?
    if (freq < 50) {
        periodLen = -1;
    }

    return periodLen;
}

void Tuner::findEpochLocations(std::vector<int> &epochLocations, float *buffer, int periodLen) {
    // This algorithm requires that the epoch locations be pretty well marked

    int largestPeak = findMaxArrayIdx(bufferIn.data(), 0, bufferSize);
    epochLocations.push_back(largestPeak);

    // First go right
    int epochCandidateIdx = epochLocations[0] + periodLen;
    while (epochCandidateIdx < bufferSize) {
        epochLocations.push_back(epochCandidateIdx);
        epochCandidateIdx += periodLen;
    }

    // Then go left
    epochCandidateIdx = epochLocations[0] - periodLen;
    while (epochCandidateIdx > 0) {
        epochLocations.push_back(epochCandidateIdx);
        epochCandidateIdx -= periodLen;
    }

    // Sort in place so that we can more easily find the period,
    // where period = (epochLocations[t+1] + epochLocations[t-1]) / 2
    std::sort(epochLocations.begin(), epochLocations.end());

    // Finally, just to make sure we have our epochs in the right
    // place, ensure that every epoch mark (sans first/last) sits on a peak
    for (int i = 1; i < epochLocations.size() - 1; i++) {
        int minIdx = epochLocations[i] - EPOCH_PEAK_REGION_WIGGLE;
        int maxIdx = epochLocations[i] + EPOCH_PEAK_REGION_WIGGLE;

        int peakOffset = findMaxArrayIdx(bufferIn.data(), minIdx, maxIdx) - minIdx;
        peakOffset -= EPOCH_PEAK_REGION_WIGGLE;

        epochLocations[i] += peakOffset;
    }
}

void Tuner::overlapAddArray(float *dest, float *src, int startIdx, int len) {
    int idxLow = startIdx;
    int idxHigh = startIdx + len;

    int padLow = 0;
    int padHigh = 0;
    if (idxLow < 0) {
        padLow = -idxLow;
    }
    if (idxHigh > bufferSize) {
        padHigh = bufferSize - idxHigh;
    }

    // Finally, reconstruct the buffer
    for (int i = padLow; i < len + padHigh; i++) {
        dest[startIdx + i] += src[i];
    }
}

int Tuner::setCurrPitchEvent(int startIdx, int bufferPos, std::vector<PitchEvent>& events) {
    for (int i = startIdx; i < events.size(); ++i) {
        if (std::abs(static_cast<int>(events[i].position - bufferPos)) < std::abs(static_cast<int>(bufferPos - currPitchEvent.position))) {
            currPitchEvent = events[i];
            return i;
        }
    }

    return -1;
}
