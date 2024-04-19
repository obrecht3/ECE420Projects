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

// TODO: CLEAN THIS UP
// Student Variables
#define EPOCH_PEAK_REGION_WIGGLE 30
#define VOICED_THRESHOLD 100000000
#define FRAME_SIZE 1024
#define BUFFER_SIZE (3 * FRAME_SIZE)
#define F_S 48000
float bufferIn[BUFFER_SIZE] = {};
float bufferOut[BUFFER_SIZE] = {};
int newEpochIdx = FRAME_SIZE;

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
    int16_t data[FRAME_SIZE];
    for (int i = 0; i < FRAME_SIZE; i++) {
        data[i] = ((uint16_t) dataBuf->buf_[2 * i]) | (((uint16_t) dataBuf->buf_[2 * i + 1]) << 8);
    }

    // PROCESS HERE
    tuner.processBlock(data);

    for (int i = 0; i < FRAME_SIZE; i++) {
        uint8_t lowByte = (uint8_t) (0x00ff & data[i]);
        uint8_t highByte = (uint8_t) ((0xff00 & data[i]) >> 8);
        dataBuf->buf_[i * 2] = lowByte;
        dataBuf->buf_[i * 2 + 1] = highByte;
    }

    gettimeofday(&end, NULL);
    LOGD("Time delay: %ld us",  ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
}

extern "C"
JNIEXPORT void JNICALL
Java_com_ece420_lab3_MainActivity_getNotesInput(JNIEnv *env, jclass clazz, jstring input) {
    // TODO: implement getNotesInput()
    // parser.parse(input)
}