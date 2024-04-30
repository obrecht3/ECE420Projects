#include "Recorder.h"
#include "Filter.h"
#include "Tuner.h"

Recorder::Recorder(int _sampleRate, int _frameSize, double _maxRecordLength)
    : sampleRate{_sampleRate}
    , frameSize{_frameSize}
    , maxRecordLength{_maxRecordLength}
    , pos{0}
    , recording{false}
    , playing{false}
    , fadeLength{24}
    , signal(static_cast<size_t>(sampleRate * maxRecordLength), 0.0f)
    , nextBuffer(frameSize){

}

Recorder::~Recorder() {
    signal.clear();
}

void Recorder::start() {
    pos = 0;
    recording = true;
    signal.resize(static_cast<size_t>(sampleRate * maxRecordLength));
    for (size_t i = 0; i < signal.size(); ++i) {
        signal[i] = 0.0f;
    }
}

void Recorder::stop() {
    pos = 0;
    if (playing) {
        nextBuffer.resize(nextBuffer.size(), 0.0f);
    }
    playing = false;

    if (recording) {
        recording = false;

        size_t startSample = findStartSample();
        size_t endSample = findEndSample();

        if (endSample - startSample > 0) {
            for (size_t i = startSample; i < endSample; ++i) {
                signal[i - startSample] = signal[i];
            }

            signal.resize(endSample - startSample);
            applyFades();
        }
    }
}

void Recorder::play() {
    pos = 0;
    recording = false;
    playing = true;
}

void Recorder::writeData(float *data) {
    for (int i = 0; i < frameSize; ++i) {
        if (pos >= signal.size()) {
            stop();
            return;
        }

        signal[pos] = data[i];
        ++pos;
    }
}

float *Recorder::getNextBuffer() {
    if (!playing) return nextBuffer.data();

    for (int i = 0; i < frameSize; ++i) {
        nextBuffer[i] = signal[pos];
        ++pos;
        if (pos >= signal.size()) {
            pos = 0;
        }
    }
    return nextBuffer.data();
}

size_t Recorder::findStartSample() {
    Tuner periodDetector(frameSize, sampleRate, 0);

    const size_t numFrames = signal.size() / frameSize;
    float data[frameSize];
    size_t signalPos = 0;

    for (size_t frame = 0; frame < numFrames; ++frame) {
        for (size_t i = 0; i < frameSize; ++i) {
            data[i] = signal[signalPos];
            signalPos++;
        }

        periodDetector.writeInputSamples(data);
        int period = periodDetector.detectBufferPeriod();

        if (period > 50) {
            return frame * frameSize;
        }
    }

    return 0;
}

size_t Recorder::findEndSample() {
    Tuner periodDetector(frameSize, sampleRate, 0);

    const size_t numFrames = signal.size() / frameSize;
    float data[frameSize];
    size_t signalPos = frameSize * numFrames;

    for (int frame = numFrames - 1; frame >= 0; --frame) {
        for (size_t i = 0; i < frameSize; ++i) {
            data[i] = signal[signalPos];
            signalPos--;
        }

        periodDetector.writeInputSamples(data);
        int period = periodDetector.detectBufferPeriod();

        if (period > 50) {
            return frame * frameSize;
        }
    }

    return 0;
}

void Recorder::applyFades() {
    for (size_t i = 0; i < fadeLength; ++i) {
        signal[pos] *= static_cast<float>(pos) / static_cast<float>(fadeLength);
    }

    for (size_t i = signal.size() - fadeLength; i < signal.size(); ++i) {
        signal[pos] *= (1.0f - static_cast<float>(pos) / static_cast<float>(fadeLength));
    }
}
