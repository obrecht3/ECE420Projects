#pragma once

#include <vector>

class Recorder {
public:
    Recorder(int sampleRate, int _frameSize, double maxRecordLength);
    ~Recorder();

    void start();
    void stop();
    void play();
    bool isRecording() { return recording; }

    void writeData(float *data);
    float* getNextBuffer();

private:
    int frameSize;
    size_t pos;
    bool recording;

    std::vector<float> signal;
    std::vector<float> nextBuffer;
};