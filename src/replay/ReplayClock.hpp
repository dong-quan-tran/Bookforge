#pragma once

#include <chrono>
#include <vector>

namespace bookforge {

class IReplayClock {
  public:
    virtual ~IReplayClock() = default;

    virtual void SleepFor(std::chrono::nanoseconds duration) = 0;
};

class WallClockReplayClock final : public IReplayClock {
  public:
    void SleepFor(std::chrono::nanoseconds duration) override;
};

class RecordingReplayClock final : public IReplayClock {
  public:
    void SleepFor(std::chrono::nanoseconds duration) override;

    [[nodiscard]] const std::vector<std::chrono::nanoseconds> &RequestedSleeps() const;
    [[nodiscard]] std::chrono::nanoseconds TotalRequestedSleep() const;

  private:
    std::vector<std::chrono::nanoseconds> requested_sleeps_;
};

} // namespace bookforge