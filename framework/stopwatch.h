#pragma once

class Stopwatch
{
public:
    Stopwatch()
    {
        QueryPerformanceFrequency(&m_ticksPerSecond);
    }

    void Start()
    {
        QueryPerformanceCounter(&m_t0);
    }

    double Stop()
    {
        LARGE_INTEGER t1;
        QueryPerformanceCounter(&t1);
        return (t1.QuadPart - m_t0.QuadPart) * 1000.0 / m_ticksPerSecond.QuadPart;
    }

    LARGE_INTEGER m_ticksPerSecond;
    LARGE_INTEGER m_t0;
};

