#include "Recorder.h"

Recorder::Recorder(int sampleRate, int _frameSize, double maxRecordLength)
    : frameSize{_frameSize}
    , pos{0}
    , recording{false}
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
    recording = false;
}

void Recorder::play() {
    pos = 0;
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
