#include "Filter.h"

#include <math.h>

SinglePoleLPF::SinglePoleLPF(double _sampleRate)
    : sampleRate{_sampleRate}
    , g{0.0}
    , gExpCoeff{2.0 * M_PI / sampleRate}
    , prevInputSample{0.0}
    , prevOutputSample{0.0} {
}

SinglePoleLPF::~SinglePoleLPF() {

}

void SinglePoleLPF::reset() {
    prevInputSample = 0.0;
    prevOutputSample = 0.0;
}

float SinglePoleLPF::processSample(float sample) {
    const double outSample = g * (sample * Coeff1 + prevInputSample * Coeff2 - prevOutputSample) + prevOutputSample;
    prevInputSample = sample;
    prevOutputSample = outSample;
    return outSample;
}

float SinglePoleLPF::calcG(float cutoff) {
    g = 1.0 - exp(-gExpCoeff * cutoff);
    return g;
}

Filter::Filter(double _sampleRate, int _bufferSize, int nCascades, double _res)
    : sampleRate{_sampleRate}
    , bufferSize{_bufferSize}
    , res{_res}
    , nonLinFeedback{1.0} {
    lpf.reserve(nCascades);
    for (int i = 0; i < nCascades; ++i) {
        lpf.emplace_back(sampleRate);
    }
}

Filter::~Filter() {
    lpf.clear();
}

void Filter::processBlock(float *data, float *envelope, std::vector<PitchEvent> pitchEvents) {
    if (lpf.size() < 1) return;

    for (int i = 0; i < bufferSize; ++i) {
        // apply filters
        const float g = lpf[0].calcG(envelope[i]);
        const float inputSample = data[i];
        float sample = lpf[0].processSample(inputSample - nonLinFeedback);
        for (int j = 1; j < lpf.size(); ++j) {
            lpf[j].setG(g);
            sample = lpf[j].processSample(sample);
        }

        // feedback loop
        nonLinFeedback = res * (4 * tanh(sample) - comp * inputSample);
        data[i] = sample;
    }
}

void Filter::reset() {
    nonLinFeedback = 1.0;
    for (auto filter : lpf) {
        filter.reset();
    }
}
