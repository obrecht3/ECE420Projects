#pragma once

#include <jni.h>
#include <math.h>
#include "ece420_lib.h"

class NoteDetector {
public:
    NoteDetector(int _bufferSize);
    ~NoteDetector();

    void reset() {
        notePlaying = false;
        triggerMelody = false;
    }
    void detect(float* data);
    bool startPlaying() { return notePlaying; }
//    bool startPlaying() { return triggerMelody; }

private:
    int bufferSize;
    bool notePlaying;
    bool triggerMelody;

    const float THRESHOLD = 1000.0f;
};