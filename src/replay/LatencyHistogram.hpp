#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace bookforge {

struct LatencyHistogramBucket {
    std::chrono::nanoseconds upper_bound{0};
    std::uint64_t count{0};
};

class LatencyHistogram {
  public:
    explicit LatencyHistogram(std::vector<std::chrono::nanoseconds> upper_bounds);

    void Record(std::chrono::nanoseconds sample);

    [[nodiscard]] const std::vector<LatencyHistogramBucket> &Buckets() const;
    [[nodiscard]] std::uint64_t OverflowCount() const;
    [[nodiscard]] std::uint64_t Count() const;
    [[nodiscard]] std::chrono::nanoseconds Total() const;
    [[nodiscard]] std::chrono::nanoseconds Min() const;
    [[nodiscard]] std::chrono::nanoseconds Max() const;
    [[nodiscard]] bool Empty() const;

  private:
    std::vector<LatencyHistogramBucket> buckets_;
    std::uint64_t overflow_count_{0};
    std::uint64_t count_{0};
    std::chrono::nanoseconds total_{0};
    std::chrono::nanoseconds min_{0};
    std::chrono::nanoseconds max_{0};
};

} // namespace bookforge