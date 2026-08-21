#include "replay/LatencyHistogram.hpp"

#include <chrono>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace bookforge {
namespace {

using namespace std::chrono_literals;

TEST(LatencyHistogramTest, RecordsSamplesAtExpectedBucketBoundaries) {
    LatencyHistogram histogram({10ns, 100ns, 1000ns});

    histogram.Record(0ns);
    histogram.Record(10ns);
    histogram.Record(11ns);
    histogram.Record(100ns);
    histogram.Record(101ns);
    histogram.Record(1000ns);
    histogram.Record(1001ns);

    const auto &buckets = histogram.Buckets();

    ASSERT_EQ(buckets.size(), 3U);
    EXPECT_EQ(buckets[0].upper_bound, 10ns);
    EXPECT_EQ(buckets[0].count, 2U);
    EXPECT_EQ(buckets[1].upper_bound, 100ns);
    EXPECT_EQ(buckets[1].count, 2U);
    EXPECT_EQ(buckets[2].upper_bound, 1000ns);
    EXPECT_EQ(buckets[2].count, 2U);

    EXPECT_EQ(histogram.OverflowCount(), 1U);
    EXPECT_EQ(histogram.Count(), 7U);
    EXPECT_EQ(histogram.Total(), 2223ns);
    EXPECT_EQ(histogram.Min(), 0ns);
    EXPECT_EQ(histogram.Max(), 1001ns);
    EXPECT_FALSE(histogram.Empty());
}

TEST(LatencyHistogramTest, IgnoresNegativeSamples) {
    LatencyHistogram histogram({10ns, 100ns});

    histogram.Record(-1ns);

    EXPECT_TRUE(histogram.Empty());
    EXPECT_EQ(histogram.Count(), 0U);
    EXPECT_EQ(histogram.Total(), 0ns);
    EXPECT_EQ(histogram.Min(), 0ns);
    EXPECT_EQ(histogram.Max(), 0ns);
    EXPECT_EQ(histogram.OverflowCount(), 0U);

    const auto &buckets = histogram.Buckets();
    ASSERT_EQ(buckets.size(), 2U);
    EXPECT_EQ(buckets[0].count, 0U);
    EXPECT_EQ(buckets[1].count, 0U);
}

TEST(LatencyHistogramTest, TracksMinMaxAndTotalAcrossBucketsAndOverflow) {
    LatencyHistogram histogram({10ns, 100ns});

    histogram.Record(5ns);
    histogram.Record(75ns);
    histogram.Record(500ns);

    EXPECT_EQ(histogram.Count(), 3U);
    EXPECT_EQ(histogram.Total(), 580ns);
    EXPECT_EQ(histogram.Min(), 5ns);
    EXPECT_EQ(histogram.Max(), 500ns);
    EXPECT_EQ(histogram.OverflowCount(), 1U);
}

TEST(LatencyHistogramTest, RejectsEmptyBucketBounds) {
    EXPECT_THROW((LatencyHistogram(std::vector<std::chrono::nanoseconds>{})),
                 std::invalid_argument);
}

TEST(LatencyHistogramTest, RejectsUnsortedBucketBounds) {
    EXPECT_THROW((LatencyHistogram(std::vector<std::chrono::nanoseconds>{100ns, 10ns})),
                 std::invalid_argument);
}

TEST(LatencyHistogramTest, RejectsDuplicateBucketBounds) {
    EXPECT_THROW((LatencyHistogram(std::vector<std::chrono::nanoseconds>{10ns, 10ns})),
                 std::invalid_argument);
}

TEST(LatencyHistogramTest, RejectsNegativeBucketBounds) {
    EXPECT_THROW((LatencyHistogram(std::vector<std::chrono::nanoseconds>{-1ns, 10ns})),
                 std::invalid_argument);
}

} // namespace
} // namespace bookforge