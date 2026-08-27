#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "HyperliquidCsvReader.hpp"
#include "HyperliquidMatchingEngineAdapter.hpp"
#include "core/matching_engine.hpp"

namespace bookforge {
namespace {

const std::string kFixturePath =
    std::string(BOOKFORGE_TEST_FIXTURE_DIR) + "/hyperliquid_lifecycle_fixture.csv";

TEST(HyperliquidLifecycleReplayTest, AppliesExplicitLifecycleEventsByExternalId) {
    HyperliquidCsvReader reader(kFixturePath);
    const std::vector<ExternalOrderEvent> events = reader.read_all(true, true);

    ASSERT_EQ(events.size(), 7U);
    EXPECT_EQ(events[3].eventType, EventType::Replace);
    EXPECT_EQ(events[6].eventType, EventType::Replace);
    ASSERT_TRUE(events[2].external_fill_size.has_value());
    EXPECT_DOUBLE_EQ(*events[2].external_fill_size, 0.00001);
    ASSERT_TRUE(events[5].external_fill_size.has_value());
    EXPECT_DOUBLE_EQ(*events[5].external_fill_size, 0.00001);

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    for (const ExternalOrderEvent &event : events) {
        adapter.OnEvent(event);
    }

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 7U);
    EXPECT_EQ(stats.newCount, 2U);
    EXPECT_EQ(stats.cancelCount, 1U);
    EXPECT_EQ(stats.fillCount, 2U);
    EXPECT_EQ(stats.replaceCount, 2U);
    EXPECT_EQ(stats.submittedOrders, 2U);
    EXPECT_EQ(stats.canceledOrders, 1U);
    EXPECT_EQ(stats.externallyFilledOrders, 2U);
    EXPECT_EQ(stats.replacedOrders, 1U);
    EXPECT_EQ(stats.ignoredEvents, 1U);
    EXPECT_EQ(adapter.Metrics().unsupported, 1U);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
}

} // namespace
} // namespace bookforge
