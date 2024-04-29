#pragma once

#include <vector>

class Recorder {
public:
    Recorder(int _sampleRate, int _frameSize, double maxRecordLength);
    ~Recorder();

    void start();
    void stop();
    void play();
    bool isRecording() { return recording; }
    bool isPlaying() { return playing; }

    void writeData(float *data);
    float* getNextBuffer();

private:
    int sampleRate;
    int frameSize;
    size_t pos;
    bool recording;
    bool playing;

    const int fadeLength;

    std::vector<float> signal;
    std::vector<float> nextBuffer;
};