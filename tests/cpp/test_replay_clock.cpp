#include "replay/ReplayClock.hpp"

#include <chrono>

#include <gtest/gtest.h>

namespace bookforge {
namespace {

TEST(RecordingReplayClockTest, RecordsPositiveSleepRequestsInOrder) {
    RecordingReplayClock clock;

    clock.SleepFor(std::chrono::nanoseconds{10});
    clock.SleepFor(std::chrono::nanoseconds{25});

    const auto &requested_sleeps = clock.RequestedSleeps();

    ASSERT_EQ(requested_sleeps.size(), 2U);
    EXPECT_EQ(requested_sleeps[0], std::chrono::nanoseconds{10});
    EXPECT_EQ(requested_sleeps[1], std::chrono::nanoseconds{25});
    EXPECT_EQ(clock.TotalRequestedSleep(), std::chrono::nanoseconds{35});
}

TEST(RecordingReplayClockTest, IgnoresZeroAndNegativeSleepRequests) {
    RecordingReplayClock clock;

    clock.SleepFor(std::chrono::nanoseconds::zero());
    clock.SleepFor(std::chrono::nanoseconds{-10});

    EXPECT_TRUE(clock.RequestedSleeps().empty());
    EXPECT_EQ(clock.TotalRequestedSleep(), std::chrono::nanoseconds::zero());
}

TEST(WallClockReplayClockTest, AcceptsZeroAndNegativeDurations) {
    WallClockReplayClock clock;

    clock.SleepFor(std::chrono::nanoseconds::zero());
    clock.SleepFor(std::chrono::nanoseconds{-1});

    SUCCEED();
}

} // namespace
} // namespace bookforge