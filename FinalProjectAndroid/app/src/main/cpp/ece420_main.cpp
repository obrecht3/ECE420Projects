//
// Created by daran on 1/12/2017 to be used in ECE420 Sp17 for the first time.
// Modified by dwang49 on 1/1/2018 to adapt to Android 7.0 and Shield Tablet updates.
//

#include <jni.h>
#include "ece420_main.h"
#include "ece420_lib.h"
#include "kiss_fft/kiss_fft.h"
#include "TextParser.h"
#include "Tuner.h"
#include "NoteDetector.h"
#include "Filter.h"
#include "EnvelopeGenerator.h"
#include "Recorder.h"

extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_writeNewTempo(JNIEnv *env, jclass, jint);
}

extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_getNotesInput(JNIEnv *env, jclass clazz, jstring input, jint melodyIdx);
}

// Student Variables
#define FRAME_SIZE 1024
#define F_S 48000

#define MAX_NUM_MELODIES 3

bool recordMode = false;

std::vector<TextParser> parser(MAX_NUM_MELODIES, TextParser(FRAME_SIZE, F_S));

Tuner tuner(FRAME_SIZE, F_S, MAX_NUM_MELODIES);
std::vector<Filter> filter(MAX_NUM_MELODIES, Filter(F_S, FRAME_SIZE, 2, 4.0, 8.0));
EnvelopeGenerator envGenerator(FRAME_SIZE);
Recorder recorder(F_S, FRAME_SIZE, 8.0);

void synthesize(const float* inputData, float *outputData) {
    std::vector<std::vector<PitchEvent>> pitchEventsList;
    std::vector<std::vector<float>> noteData(MAX_NUM_MELODIES, std::vector<float>(FRAME_SIZE, 0));

    tuner.writeInputSamples(inputData);
    const int period = tuner.detectBufferPeriod();
    const float userFreq = static_cast<float>(F_S) / static_cast<float>(period);

    for (auto& p : parser) {
        p.calcPitchEvents(userFreq);
        pitchEventsList.push_back(p.getPitchEventsForNextBuffer());
    }

    tuner.processBlock(noteData, pitchEventsList, period);

    envGenerator.setSamplesPerNote(parser[0].getSamplesPerNote());
    const float *envelope = envGenerator.getNextBlock(pitchEventsList[0]);

    for (int melodyIdx = 0; melodyIdx < noteData.size(); melodyIdx++) {
        filter[melodyIdx].processBlock(noteData[melodyIdx].data(), envelope, pitchEventsList[melodyIdx]);

        for (int i = 0; i < FRAME_SIZE; i++) {
            outputData[i] += noteData[melodyIdx][i];
        }
    }

    for (int i = 0; i < FRAME_SIZE; i++) {
        outputData[i] *= envelope[i];
    }
}

void ece420ProcessFrame(sample_buf *dataBuf) {
    // Keep in mind, we only have 20ms to process each buffer!
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    // Data is encoded in signed PCM-16, little-endian, mono
    float data[FRAME_SIZE];
    for (int i = 0; i < FRAME_SIZE; i++) {
        const int16_t value = ((uint16_t) dataBuf->buf_[2 * i]) | (((uint16_t) dataBuf->buf_[2 * i + 1]) << 8);
        data[i] = value;
    }

    if (recordMode && recorder.isRecording()) {
        recorder.writeData(data);
    } else {
        synthesize(recorder.getNextBuffer(), data);
    }

    for (int i = 0; i < FRAME_SIZE; i++) {
        const int16_t value = data[i];
        uint8_t lowByte = (uint8_t) (0x00ff & value);
        uint8_t highByte = (uint8_t) ((0xff00 & value) >> 8);
        dataBuf->buf_[i * 2] = lowByte;
        dataBuf->buf_[i * 2 + 1] = highByte;
    }

    gettimeofday(&end, NULL);
    LOGD("Time delay: %ld us",  ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));

}

extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_writeNewTempo(JNIEnv *env, jclass, jint newTempo) {
    const int tempo = std::max(40, newTempo);
    for (auto& p : parser)
        p.setTempo(tempo);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_writeNewEnvelopePeakPosition(JNIEnv *env, jclass, jint newEnvelopePeakPosition) {
    envGenerator.setShape(static_cast<float>(newEnvelopePeakPosition) / 100.0f, parser[0].getSamplesPerNote());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_getNotesInput(JNIEnv *env, jclass clazz, jstring input, jint melodyIdx) {
    const char *cstr = env->GetStringUTFChars(input, NULL);
    std::string str = std::string(cstr);
    env->ReleaseStringUTFChars(input, cstr);
    parser[melodyIdx].parse(str);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_startRecord(JNIEnv *env, jclass clazz) {
    recorder.start();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_stopRecord(JNIEnv *env, jclass clazz) {
    recorder.stop();
}
extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_setRecordMode(JNIEnv *env, jclass clazz,
                                                jboolean record_mode_on) {
    recordMode = record_mode_on;
}