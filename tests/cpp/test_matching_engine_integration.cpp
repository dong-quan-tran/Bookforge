#include <gtest/gtest.h>

#include "core/matching_engine.hpp"
#include "core/order.hpp"

namespace bookforge {
namespace {

Order MakeOrder(std::uint64_t id, Side side, double price, std::uint32_t qty, std::uint64_t ts,
                std::uint64_t participant = 0,
                SelfTradePrevention stp = SelfTradePrevention::None) {
    Order o{};
    o.id = id;
    o.participant_id = participant;
    o.side = side;
    o.price = price;
    o.quantity = qty;
    o.timestamp = ts;
    o.stp = stp;
    return o;
}

TEST(MatchingEngineIntegrationTest, PassiveOrderInsertionBuildsTopOfBook) {
    MatchingEngine engine;

    auto r1 = engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));
    auto r2 = engine.MatchLimitOrder(MakeOrder(2, Side::Sell, 101.0, 7, 2));

    EXPECT_TRUE(r1.trades.empty());
    EXPECT_TRUE(r2.trades.empty());

    auto top = engine.CaptureTopOfBook();
    ASSERT_TRUE(top.best_bid.has_value());
    ASSERT_TRUE(top.best_ask.has_value());
    EXPECT_DOUBLE_EQ(*top.best_bid, 100.0);
    EXPECT_DOUBLE_EQ(*top.best_ask, 101.0);
    ASSERT_TRUE(top.best_bid_volume.has_value());
    ASSERT_TRUE(top.best_ask_volume.has_value());
    EXPECT_EQ(*top.best_bid_volume, 10u);
    EXPECT_EQ(*top.best_ask_volume, 7u);
}

TEST(MatchingEngineIntegrationTest, AggressiveExecutionGeneratesTradeAtRestingPrice) {
    MatchingEngine engine;

    engine.MatchLimitOrder(MakeOrder(1, Side::Sell, 101.0, 5, 1));
    auto result = engine.MatchLimitOrder(MakeOrder(2, Side::Buy, 105.0, 5, 2));

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].taker_order_id, 2u);
    EXPECT_EQ(result.trades[0].maker_order_id, 1u);
    EXPECT_EQ(result.trades[0].side, Side::Buy);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 101.0);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    auto top = engine.CaptureTopOfBook();
    EXPECT_FALSE(top.best_ask.has_value());
}

TEST(MatchingEngineIntegrationTest, PartialFillLeavesRemainingMakerQuantity) {
    MatchingEngine engine;

    engine.MatchLimitOrder(MakeOrder(1, Side::Sell, 101.0, 10, 1));
    auto result = engine.MatchLimitOrder(MakeOrder(2, Side::Buy, 101.0, 4, 2));

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].quantity, 4u);

    auto top = engine.CaptureTopOfBook();
    ASSERT_TRUE(top.best_ask.has_value());
    EXPECT_DOUBLE_EQ(*top.best_ask, 101.0);
    ASSERT_TRUE(top.best_ask_volume.has_value());
    EXPECT_EQ(*top.best_ask_volume, 6u);
}

TEST(MatchingEngineIntegrationTest, PartialSweepRestsRemainingTakerQuantityWhenBookClears) {
    MatchingEngine engine;

    engine.MatchLimitOrder(MakeOrder(1, Side::Sell, 101.0, 3, 1));
    auto result = engine.MatchLimitOrder(MakeOrder(2, Side::Buy, 105.0, 8, 2));

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].quantity, 3u);

    auto top = engine.CaptureTopOfBook();
    ASSERT_TRUE(top.best_bid.has_value());
    EXPECT_DOUBLE_EQ(*top.best_bid, 105.0);
    ASSERT_TRUE(top.best_bid_volume.has_value());
    EXPECT_EQ(*top.best_bid_volume, 5u);
    EXPECT_FALSE(top.best_ask.has_value());
}

TEST(MatchingEngineIntegrationTest, CancelAfterPartialFillRemovesRemainingRestingQuantity) {
    MatchingEngine engine;

    engine.MatchLimitOrder(MakeOrder(10, Side::Sell, 101.0, 10, 1));
    engine.MatchLimitOrder(MakeOrder(20, Side::Buy, 101.0, 4, 2));

    auto top_before_cancel = engine.CaptureTopOfBook();
    ASSERT_TRUE(top_before_cancel.best_ask_volume.has_value());
    EXPECT_EQ(*top_before_cancel.best_ask_volume, 6u);

    const bool canceled = engine.CancelOrder(10);
    EXPECT_TRUE(canceled);

    auto top_after_cancel = engine.CaptureTopOfBook();
    EXPECT_FALSE(top_after_cancel.best_ask.has_value());
}

TEST(MatchingEngineIntegrationTest, ReplayStyleSequenceProducesConsistentTopOfBookEvolution) {
    MatchingEngine engine;

    std::vector<TopOfBookSnapshot> snapshots;

    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 5, 1));
    snapshots.push_back(engine.CaptureTopOfBook());

    engine.MatchLimitOrder(MakeOrder(2, Side::Buy, 99.0, 4, 2));
    snapshots.push_back(engine.CaptureTopOfBook());

    engine.MatchLimitOrder(MakeOrder(3, Side::Sell, 101.0, 8, 3));
    snapshots.push_back(engine.CaptureTopOfBook());

    engine.MatchLimitOrder(MakeOrder(4, Side::Buy, 101.0, 3, 4));
    snapshots.push_back(engine.CaptureTopOfBook());

    ASSERT_EQ(snapshots.size(), 4u);

    ASSERT_TRUE(snapshots[0].best_bid.has_value());
    EXPECT_DOUBLE_EQ(*snapshots[0].best_bid, 100.0);
    EXPECT_FALSE(snapshots[0].best_ask.has_value());

    ASSERT_TRUE(snapshots[1].best_bid.has_value());
    EXPECT_DOUBLE_EQ(*snapshots[1].best_bid, 100.0);

    ASSERT_TRUE(snapshots[2].best_ask.has_value());
    EXPECT_DOUBLE_EQ(*snapshots[2].best_ask, 101.0);

    ASSERT_TRUE(snapshots[3].best_ask_volume.has_value());
    EXPECT_EQ(*snapshots[3].best_ask_volume, 5u);
}

TEST(MatchingEngineIntegrationTest, EventLogCapturesAcceptTradeRestAndCancelFlow) {
    MatchingEngine engine;

    engine.MatchLimitOrder(MakeOrder(1, Side::Sell, 101.0, 6, 1));
    engine.MatchLimitOrder(MakeOrder(2, Side::Buy, 101.0, 4, 2));
    engine.CancelOrder(1);

    const auto &log = engine.EventLog();
    ASSERT_GE(log.size(), 4u);

    EXPECT_EQ(log[0].type, EngineEventType::Accepted);
    EXPECT_EQ(log[1].type, EngineEventType::Rested);
    EXPECT_EQ(log[2].type, EngineEventType::Accepted);
}

} // namespace
} // namespace bookforge