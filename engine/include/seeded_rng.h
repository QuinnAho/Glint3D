#pragma once
#include <random>

class SeededRng {
public:
    explicit SeededRng(uint32_t seed = 0) : m_rng(seed) {}
    void setSeed(uint32_t seed) { m_rng.seed(seed); }
    float uniform() { return m_uniform(m_rng); }
    float stratified(int sample, int total, float jitter = 0.0f) {
        float base = (float(sample) + jitter) / float(total);
        return base < 1.0f ? base : 0.999999f;
    }
private:
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_uniform{0.0f, 1.0f};
};
