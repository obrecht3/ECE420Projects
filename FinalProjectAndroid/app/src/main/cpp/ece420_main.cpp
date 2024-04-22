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

extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_writeNewTempo(JNIEnv *env, jclass, jint);
}

extern "C" {
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_getNotesInput(JNIEnv *env, jclass clazz, jstring input);
}

// Student Variables
#define EPOCH_PEAK_REGION_WIGGLE 30
#define VOICED_THRESHOLD 100000000
#define FRAME_SIZE 1024
#define BUFFER_SIZE (3 * FRAME_SIZE)
#define F_S 48000
float bufferIn[BUFFER_SIZE] = {};
float bufferOut[BUFFER_SIZE] = {};
int newEpochIdx = FRAME_SIZE;

double attackIncrement;
double decayIncrement;

// We have two variables here to ensure that we never change the desired frequency while
// processing a frame. Thread synchronization, etc. Setting to 300 is only an initializer.
int FREQ_NEW_ANDROID = 300;
int FREQ_NEW = 300;
// Switch from UI (Use this to enable/disable TDPSOLA to display the spectrogram
bool IS_TDPSOLA_ANDROID = false;

// FRAME_SIZE is 1024 and we zero-pad it to 2048 to do FFT
#define ZP_FACTOR 2
#define FFT_SIZE (FRAME_SIZE * ZP_FACTOR)
// Variable to store final FFT output
float fftOut[FFT_SIZE] = {};
bool isWritingFft = false;

// FFT Vars
kiss_fft_cfg kfftCfg = kiss_fft_alloc(FFT_SIZE, false, NULL,NULL);
kiss_fft_cpx kfftIn[FFT_SIZE] = {};
kiss_fft_cpx kfftOut[FFT_SIZE] = {};

TextParser parser(FRAME_SIZE, F_S);
Tuner tuner(FRAME_SIZE, F_S);
NoteDetector noteDetector(FRAME_SIZE);

void processFFT(float* in, float* out) {
    // 1. Apply hamming window to the entire FRAME_SIZE
    for (int i = 0; i < FRAME_SIZE; i++) {
        kfftIn[i] = { in[i] * getHanningCoef(FRAME_SIZE,i), 0.0f };
    }

    // 2. Zero padding to FFT_SIZE = FRAME_SIZE * ZP_FACTOR
    // probably not necessary
    for (int i = FRAME_SIZE; i < FFT_SIZE; i++) {
        kfftIn[i] = {0.0f, 0.0f};
    }

    // 3. Apply fft with KISS_FFT engine
    kiss_fft(kfftCfg, kfftIn, kfftOut);

    // 4. Scale fftOut[] to between 0 and 1 with log() and linear scaling
    for (int i = 0; i < FFT_SIZE; i++) {
        const float magSquared = kfftOut[i].r * kfftOut[i].r + kfftOut[i].i + kfftOut[i].i;
        out[i] = log10(magSquared) * 0.0000001;
    }
}

void ece420ProcessFrame(sample_buf *dataBuf) {
    isWritingFft = false;

    // Keep in mind, we only have 20ms to process each buffer!
    struct timeval start;
    struct timeval end;
    gettimeofday(&start, NULL);

    // Get the new desired frequency from android
    FREQ_NEW = FREQ_NEW_ANDROID;


    // Data is encoded in signed PCM-16, little-endian, mono
    float data[FRAME_SIZE];
    for (int i = 0; i < FRAME_SIZE; i++) {
        const int16_t value = ((uint16_t) dataBuf->buf_[2 * i]) | (((uint16_t) dataBuf->buf_[2 * i + 1]) << 8);
        data[i] = value;
    }

    // PROCESS HERE
//    noteDetector.detect(data);
    tuner.writeInputSamples(data);
    const int period = tuner.detectBufferPeriod();
    parser.calcPitchEvents(220.0f);
//    if (noteDetector.startPlaying()) {
//        const float userFreq = static_cast<float>(F_S) / static_cast<float>(period);
//        parser.calcPitchEvents(userFreq);
//    }

    std::vector<PitchEvent> pitchEvents = parser.getPitchEventsForNextBuffer();
    tuner.processBlock(data, pitchEvents, period);
//    if (parser.melodyDone()) {
//        noteDetector.reset();
//    }

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

std::vector<double> envelopeCreate(double attackIncrement, double decayIncrement, int newEnvelopePeakPosition){
    std::vector<double> envelope;
    int samplesPerNote = parser.getSamplesPerNote();
    for(int i = 0; i >= samplesPerNote; i++){
        if(i <= newEnvelopePeakPosition/100 * samplesPerNote){
            envelope.emplace_back(attackIncrement * i);
        } else {
            envelope.emplace_back(1 - decayIncrement * (i - newEnvelopePeakPosition/100 * samplesPerNote));
        }
    }
    return envelope;
}



extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_writeNewTempo(JNIEnv *env, jclass, jint newTempo) {
    parser.setTempo(newTempo);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_writeNewEnvelopePeakPosition(JNIEnv *env, jclass, jint newEnvelopePeakPosition) {
    attackIncrement = 100/(parser.getSamplesPerNote() * newEnvelopePeakPosition);
    decayIncrement = -100/(parser.getSamplesPerNote() * newEnvelopePeakPosition);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_getNotesInput(JNIEnv *env, jclass clazz, jstring input) {
    const char *cstr = env->GetStringUTFChars(input, NULL);
    std::string str = std::string(cstr);
    env->ReleaseStringUTFChars(input, cstr);
    parser.parse(str);
}