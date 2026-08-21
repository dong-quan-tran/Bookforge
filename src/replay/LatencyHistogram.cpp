#include "replay/LatencyHistogram.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace bookforge {

LatencyHistogram::LatencyHistogram(std::vector<std::chrono::nanoseconds> upper_bounds) {
    if (upper_bounds.empty()) {
        throw std::invalid_argument("LatencyHistogram requires at least one bucket upper bound.");
    }

    if (!std::is_sorted(upper_bounds.begin(), upper_bounds.end())) {
        throw std::invalid_argument(
            "LatencyHistogram bucket upper bounds must be sorted in ascending order.");
    }

    if (upper_bounds.front() < std::chrono::nanoseconds::zero()) {
        throw std::invalid_argument("LatencyHistogram bucket upper bounds must be non-negative.");
    }

    for (std::size_t i = 1; i < upper_bounds.size(); ++i) {
        if (upper_bounds[i] <= upper_bounds[i - 1]) {
            throw std::invalid_argument(
                "LatencyHistogram bucket upper bounds must be strictly increasing.");
        }
    }

    buckets_.reserve(upper_bounds.size());

    for (const auto upper_bound : upper_bounds) {
        buckets_.push_back(LatencyHistogramBucket{
            .upper_bound = upper_bound,
            .count = 0,
        });
    }
}

void LatencyHistogram::Record(std::chrono::nanoseconds sample) {
    if (sample < std::chrono::nanoseconds::zero()) {
        return;
    }

    if (count_ == 0) {
        min_ = sample;
        max_ = sample;
    } else {
        min_ = std::min(min_, sample);
        max_ = std::max(max_, sample);
    }

    ++count_;
    total_ += sample;

    const auto bucket_it =
        std::lower_bound(buckets_.begin(), buckets_.end(), sample,
                         [](const LatencyHistogramBucket &bucket, std::chrono::nanoseconds value) {
                             return bucket.upper_bound < value;
                         });

    if (bucket_it == buckets_.end()) {
        ++overflow_count_;
        return;
    }

    ++bucket_it->count;
}

const std::vector<LatencyHistogramBucket> &LatencyHistogram::Buckets() const {
    return buckets_;
}

std::uint64_t LatencyHistogram::OverflowCount() const {
    return overflow_count_;
}

std::uint64_t LatencyHistogram::Count() const {
    return count_;
}

std::chrono::nanoseconds LatencyHistogram::Total() const {
    return total_;
}

std::chrono::nanoseconds LatencyHistogram::Min() const {
    return min_;
}

std::chrono::nanoseconds LatencyHistogram::Max() const {
    return max_;
}

bool LatencyHistogram::Empty() const {
    return count_ == 0;
}

} // namespace bookforge