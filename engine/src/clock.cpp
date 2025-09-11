#include "clock.h"
#include <chrono>

using namespace std::chrono;

double SystemClock::now() const {
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

FixedTimestepClock::FixedTimestepClock(int timestepMs)
    : m_step(static_cast<double>(timestepMs) / 1000.0) {}

double FixedTimestepClock::now() const {
    m_time += m_step;
    return m_time;
}
