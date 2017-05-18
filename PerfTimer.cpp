#include "PerfTimer.h"

using namespace std;
using namespace std::chrono;

void PerfTimer::Start() {
    startTime = high_resolution_clock::now();
}

double PerfTimer::Stop() {
    auto endTime = high_resolution_clock::now();
    return duration_cast<duration<double>>(endTime - startTime).count();
}
