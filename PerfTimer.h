#pragma once

#include <chrono>

class PerfTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime;

public:
    // Starts the timer
    void Start();

    // Stops the timer, returning the elapsed time since start in seconds.
    double Stop();
};

