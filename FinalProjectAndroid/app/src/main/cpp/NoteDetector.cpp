//
// Created by joels on 4/18/2024.
//

#include "NoteDetector.h"

NoteDetector::NoteDetector(int _bufferSize)
    : bufferSize{_bufferSize}
    , notePlaying{false} {

}

NoteDetector::~NoteDetector() {

}

void NoteDetector::detect(float *data) {
    if (notePlaying) {
        if (triggerMelody) triggerMelody = false;
        return;
    }

    float sum = 0.0f;
    for (int i = 0; i < bufferSize; ++i) {
        sum += std::abs(data[i]);
    }

    if (sum > bufferSize * THRESHOLD) {
        notePlaying = true;
        triggerMelody = true;
    }
}
