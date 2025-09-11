#pragma once

class IClock {
public:
    virtual ~IClock() = default;
    virtual double now() const = 0;
};

class SystemClock : public IClock {
public:
    double now() const override;
};

class FixedTimestepClock : public IClock {
public:
    explicit FixedTimestepClock(int timestepMs);
    double now() const override;
private:
    mutable double m_time = 0.0;
    double m_step;
};
