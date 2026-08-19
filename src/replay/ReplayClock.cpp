#include "replay/ReplayClock.hpp"

#include <chrono>
#include <thread>

namespace bookforge {

void WallClockReplayClock::SleepFor(std::chrono::nanoseconds duration) {
    if (duration <= std::chrono::nanoseconds::zero()) {
        return;
    }

    std::this_thread::sleep_for(duration);
}

void RecordingReplayClock::SleepFor(std::chrono::nanoseconds duration) {
    if (duration <= std::chrono::nanoseconds::zero()) {
        return;
    }

    requested_sleeps_.push_back(duration);
}

const std::vector<std::chrono::nanoseconds> &RecordingReplayClock::RequestedSleeps() const {
    return requested_sleeps_;
}

std::chrono::nanoseconds RecordingReplayClock::TotalRequestedSleep() const {
    std::chrono::nanoseconds total{0};

    for (const auto duration : requested_sleeps_) {
        total += duration;
    }

    return total;
}

} // namespace bookforge