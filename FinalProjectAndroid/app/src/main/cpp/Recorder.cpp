#include "Recorder.h"
#include "Filter.h"

Recorder::Recorder(int _sampleRate, int _frameSize, double maxRecordLength)
    : sampleRate{_sampleRate}
    , frameSize{_frameSize}
    , pos{0}
    , recording{false}
    , playing{false}
    , fadeLength{24}
    , signal(static_cast<int>(sampleRate * maxRecordLength), 0.0f)
    , nextBuffer(frameSize){

}

Recorder::~Recorder() {
    signal.clear();
}

void Recorder::start() {
    pos = 0;
    recording = true;
    signal.resize(signal.size(), 0.0f);
}

void Recorder::stop() {
    pos = 0;
    if (playing) {
        nextBuffer.resize(nextBuffer.size(), 0.0f);
    }
    playing = false;

    if (recording) {
        recording = false;

        // process signal

        // find envelope
        std::vector<float> envelope(signal.size());
        std::vector<SinglePoleLPF> filter(4, SinglePoleLPF(sampleRate));
        for (auto f : filter) {
            f.setG(f.calcG(5));
        }

        float magnitude = 0.0f;
        for (int i = 0; i < signal.size(); ++i) {
            float sample = std::abs(signal[i]);
            for (auto f : filter) {
                sample = f.processSample(sample);
            }
            envelope[i] = sample;
            magnitude = std::max(magnitude, sample);
        }

//        // find start and end of note
//        int startSample = 0;
//        for (int i = 0; i < signal.size(); ++i) {
//
//        }
    }
}

void Recorder::play() {
    pos = 0;
    recording = false;
    playing = true;
}

void Recorder::writeData(float *data) {
    const int fadeOutSample = signal.size() - fadeLength;
    for (int i = 0; i < frameSize; ++i) {
        if (pos >= signal.size()) {
            stop();
            return;
        } else if (pos <= fadeLength) {
            signal[pos] = data[i] * static_cast<float>(pos) / static_cast<float>(fadeLength);
        } else if (pos >= fadeOutSample) {
            signal[pos] = data[i] * (1.0f - static_cast<float>(pos - fadeOutSample) / static_cast<float>(fadeLength));
        } else {
            signal[pos] = data[i];
        }
        ++pos;
    }
}

float *Recorder::getNextBuffer() {
    if (!playing) return nextBuffer.data();

    for (int i = 0; i < frameSize; ++i) {
        if (pos >= signal.size()) {
            nextBuffer[i] = 0.0f;
        } else {
            nextBuffer[i] = signal[pos];
            ++pos;
        }
    }
    return nextBuffer.data();
}
