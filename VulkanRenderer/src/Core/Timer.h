#pragma once

#include <chrono>

class Timer
{
private:
    std::chrono::time_point<std::chrono::steady_clock> m_Start = std::chrono::steady_clock::now();

public:
    inline void Reset() { m_Start = std::chrono::steady_clock::now(); }

    inline double ElapsedTime() const { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_Start).count() / 1000.0f; }
};