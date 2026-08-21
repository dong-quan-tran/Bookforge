#include "replay/ReplayMetricsReporter.hpp"

#include <chrono>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace bookforge {
namespace {

TEST(ReplayMetricsReporterTest, WritesNothingForEmptyHistogram) {
    ReplayRunMetrics metrics{
        .requested_pacing_delays = LatencyHistogram(std::vector<std::chrono::nanoseconds>{
            std::chrono::nanoseconds{10},
            std::chrono::nanoseconds{100},
        }),
    };

    std::ostringstream output;
    WriteReplayPacingSummary(output, metrics);

    EXPECT_TRUE(output.str().empty());
}

TEST(ReplayMetricsReporterTest, WritesSummaryAndBucketCounts) {
    ReplayRunMetrics metrics{
        .requested_pacing_delays = LatencyHistogram(std::vector<std::chrono::nanoseconds>{
            std::chrono::nanoseconds{10},
            std::chrono::nanoseconds{100},
        }),
    };

    metrics.requested_pacing_delays.Record(std::chrono::nanoseconds{5});
    metrics.requested_pacing_delays.Record(std::chrono::nanoseconds{75});
    metrics.requested_pacing_delays.Record(std::chrono::nanoseconds{500});

    std::ostringstream output;
    WriteReplayPacingSummary(output, metrics);

    const std::string text = output.str();

    EXPECT_NE(text.find("Requested pacing delays:\n"), std::string::npos);
    EXPECT_NE(text.find("samples: 3"), std::string::npos);
    EXPECT_NE(text.find("total_ns: 580"), std::string::npos);
    EXPECT_NE(text.find("min_ns: 5"), std::string::npos);
    EXPECT_NE(text.find("max_ns: 500"), std::string::npos);
    EXPECT_NE(text.find("overflow: 1"), std::string::npos);
    EXPECT_NE(text.find("<= 10 ns: 1"), std::string::npos);
    EXPECT_NE(text.find("<= 100 ns: 1"), std::string::npos);
}

} // namespace
} // namespace bookforge