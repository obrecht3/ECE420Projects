#pragma once

#include <vector>

class SinglePoleLPF {
public:
    SinglePoleLPF(double _sampleRate);
    ~SinglePoleLPF();

    void reset();
    float calcG(float cutoff);
    void setG(float newG) { g = newG; }
    float processSample(float sample);

private:
    double sampleRate;

    double g;
    const double gExpCoeff;
    const double Coeff1 = 1.0 / 1.3;
    const double Coeff2 = 0.3 / 1.3;

    double prevInputSample;
    double prevOutputSample;
};

class Filter {
public:
    Filter(double _sampleRate, int _bufferSize, int nCascades, double _res = 0.707);
    ~Filter();

    void processBlock(float *data, float *envelope);
    void setRes(double newRes) { res = newRes; }
    void reset();

private:
    double sampleRate;
    int bufferSize;

    std::vector<SinglePoleLPF> lpf;

    const double comp = 0.5;
    double res;
    double nonLinFeedback;
};