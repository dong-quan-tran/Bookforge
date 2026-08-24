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

TEST(HyperliquidLifecycleReplayTest, CancelsOnlyMappedIdBearingRestingOrder) {
    HyperliquidCsvReader reader(kFixturePath);
    const std::vector<ExternalOrderEvent> events = reader.read_all(true, true);

    ASSERT_EQ(events.size(), 5U);
    EXPECT_EQ(events[0].external_order_id, "btc-ask-1");
    EXPECT_EQ(events[1].external_order_id, "eth-bid-1");

    MatchingEngine engine;
    HyperliquidMatchingEngineAdapter adapter(engine);

    for (const ExternalOrderEvent &event : events) {
        adapter.OnEvent(event);
    }

    const ReplayStats &stats = adapter.Stats();
    EXPECT_EQ(stats.totalEvents, 5U);
    EXPECT_EQ(stats.newCount, 2U);
    EXPECT_EQ(stats.cancelCount, 2U);
    EXPECT_EQ(stats.fillCount, 1U);
    EXPECT_EQ(stats.submittedOrders, 2U);
    EXPECT_EQ(stats.canceledOrders, 1U);
    EXPECT_EQ(stats.ignoredEvents, 2U);
    EXPECT_EQ(adapter.Metrics().unsupported, 2U);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());

    const auto best_bid = engine.Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 90.0);
}

} // namespace
} // namespace bookforge
