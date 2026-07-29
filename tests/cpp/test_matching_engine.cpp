#include <gtest/gtest.h>

#include "core/matching_engine.hpp"

using namespace bookforge;

namespace {

Order MakeOrder(std::uint64_t id, std::uint64_t participant_id, Side side, double price,
                std::uint32_t quantity, std::uint64_t timestamp,
                SelfTradePrevention stp = SelfTradePrevention::None) {
    return Order{id, participant_id, side, price, quantity, timestamp, stp};
}

} // namespace

TEST(MatchingEngineTest, NonCrossingLimitOrderRestsInBook) {
    MatchingEngine engine;

    Order buy = MakeOrder(1, 1, Side::Buy, 100.00, 10, 1);

    auto result = engine.MatchLimitOrder(buy);
    EXPECT_TRUE(result.trades.empty());
    EXPECT_EQ(result.remaining_quantity, 0u);

    auto best_bid = engine.Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 100.00);
}

TEST(MatchingEngineTest, CrossingBuyMatchesBestAskAtOrBetterPrice) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Sell, 100.50, 10, 1)));

    Order incoming = MakeOrder(2, 2, Side::Buy, 101.00, 5, 2);

    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    const auto &trade = result.trades[0];
    EXPECT_EQ(trade.taker_order_id, 2u);
    EXPECT_EQ(trade.maker_order_id, 1u);
    EXPECT_EQ(trade.side, Side::Buy);
    EXPECT_DOUBLE_EQ(trade.price, 100.50);
    EXPECT_EQ(trade.quantity, 5u);

    auto volume = engine.Book().GetLevelVolume(Side::Sell, 100.50);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 5u);
}

TEST(MatchingEngineTest, CrossingSellMatchesBestBidAtOrBetterPrice) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));

    Order incoming = MakeOrder(2, 2, Side::Sell, 99.50, 4, 2);

    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    const auto &trade = result.trades[0];
    EXPECT_EQ(trade.taker_order_id, 2u);
    EXPECT_EQ(trade.maker_order_id, 1u);
    EXPECT_EQ(trade.side, Side::Sell);
    EXPECT_DOUBLE_EQ(trade.price, 100.00);
    EXPECT_EQ(trade.quantity, 4u);

    auto volume = engine.Book().GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 6u);
}

TEST(MatchingEngineTest, CrossingBuyPartiallyFillsAndRestsRemainder) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Sell, 100.50, 5, 1)));

    Order incoming = MakeOrder(2, 2, Side::Buy, 101.00, 10, 2);

    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].maker_order_id, 1u);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    auto best_bid = engine.Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 101.00);

    auto volume = engine.Book().GetLevelVolume(Side::Buy, 101.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 5u);
}

TEST(MatchingEngineTest, CrossingBuyMatchesAcrossMultipleAskLevels) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Sell, 100.50, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(2, 2, Side::Sell, 100.75, 4, 2)));

    Order incoming = MakeOrder(3, 3, Side::Buy, 101.00, 8, 3);

    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 2u);

    EXPECT_EQ(result.trades[0].taker_order_id, 3u);
    EXPECT_EQ(result.trades[0].maker_order_id, 1u);
    EXPECT_EQ(result.trades[0].side, Side::Buy);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 100.50);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    EXPECT_EQ(result.trades[1].taker_order_id, 3u);
    EXPECT_EQ(result.trades[1].maker_order_id, 2u);
    EXPECT_EQ(result.trades[1].side, Side::Buy);
    EXPECT_DOUBLE_EQ(result.trades[1].price, 100.75);
    EXPECT_EQ(result.trades[1].quantity, 3u);

    EXPECT_FALSE(engine.Book().GetLevelVolume(Side::Sell, 100.50).has_value());

    auto remaining_ask = engine.Book().GetLevelVolume(Side::Sell, 100.75);
    ASSERT_TRUE(remaining_ask.has_value());
    EXPECT_EQ(*remaining_ask, 1u);
}

TEST(MatchingEngineTest, CrossingSellMatchesAcrossMultipleBidLevels) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(2, 2, Side::Buy, 99.75, 4, 2)));

    Order incoming = MakeOrder(3, 3, Side::Sell, 99.50, 8, 3);

    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 2u);

    EXPECT_EQ(result.trades[0].taker_order_id, 3u);
    EXPECT_EQ(result.trades[0].maker_order_id, 1u);
    EXPECT_EQ(result.trades[0].side, Side::Sell);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 100.00);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    EXPECT_EQ(result.trades[1].taker_order_id, 3u);
    EXPECT_EQ(result.trades[1].maker_order_id, 2u);
    EXPECT_EQ(result.trades[1].side, Side::Sell);
    EXPECT_DOUBLE_EQ(result.trades[1].price, 99.75);
    EXPECT_EQ(result.trades[1].quantity, 3u);

    EXPECT_FALSE(engine.Book().GetLevelVolume(Side::Buy, 100.00).has_value());

    auto remaining_bid = engine.Book().GetLevelVolume(Side::Buy, 99.75);
    ASSERT_TRUE(remaining_bid.has_value());
    EXPECT_EQ(*remaining_bid, 1u);
}

TEST(MatchingEngineTest, ResidualBuyRestsAfterExhaustingMarketableAskLevels) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Sell, 100.50, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(2, 2, Side::Sell, 100.75, 4, 2)));

    Order incoming = MakeOrder(3, 3, Side::Buy, 100.75, 12, 3);

    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 2u);
    EXPECT_EQ(result.trades[0].maker_order_id, 1u);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 100.50);
    EXPECT_EQ(result.trades[0].quantity, 5u);
    EXPECT_EQ(result.trades[1].maker_order_id, 2u);
    EXPECT_DOUBLE_EQ(result.trades[1].price, 100.75);
    EXPECT_EQ(result.trades[1].quantity, 4u);

    auto resting_bid = engine.Book().GetLevelVolume(Side::Buy, 100.75);
    ASSERT_TRUE(resting_bid.has_value());
    EXPECT_EQ(*resting_bid, 3u);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(MatchingEngineTest, TradeReportsMakerOrderIdForSingleFill) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(10, 10, Side::Sell, 100.50, 5, 1)));

    Order incoming = MakeOrder(20, 20, Side::Buy, 101.00, 5, 2);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].taker_order_id, 20u);
    EXPECT_EQ(result.trades[0].maker_order_id, 10u);
    EXPECT_EQ(result.trades[0].side, Side::Buy);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 100.50);
    EXPECT_EQ(result.trades[0].quantity, 5u);
}

TEST(MatchingEngineTest, TradeReportsMultipleMakerOrderIdsAtSamePriceLevel) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(10, 10, Side::Sell, 100.50, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(11, 11, Side::Sell, 100.50, 4, 2)));

    Order incoming = MakeOrder(20, 20, Side::Buy, 101.00, 8, 3);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 2u);

    EXPECT_EQ(result.trades[0].maker_order_id, 10u);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 100.50);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    EXPECT_EQ(result.trades[1].maker_order_id, 11u);
    EXPECT_DOUBLE_EQ(result.trades[1].price, 100.50);
    EXPECT_EQ(result.trades[1].quantity, 3u);

    auto remaining = engine.Book().GetLevelVolume(Side::Sell, 100.50);
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(*remaining, 1u);
}

TEST(MatchingEngineTest, TradeReportsMakerOrderIdsAcrossMultipleLevels) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(10, 10, Side::Sell, 100.50, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(11, 11, Side::Sell, 100.75, 4, 2)));

    Order incoming = MakeOrder(20, 20, Side::Buy, 101.00, 8, 3);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 2u);

    EXPECT_EQ(result.trades[0].maker_order_id, 10u);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 100.50);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    EXPECT_EQ(result.trades[1].maker_order_id, 11u);
    EXPECT_DOUBLE_EQ(result.trades[1].price, 100.75);
    EXPECT_EQ(result.trades[1].quantity, 3u);
}

TEST(MatchingEngineTest, PartialFillKeepsMakerOrderInBookWithReducedQuantity) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(10, 10, Side::Sell, 100.50, 7, 1)));

    Order incoming = MakeOrder(20, 20, Side::Buy, 101.00, 3, 2);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].maker_order_id, 10u);
    EXPECT_EQ(result.trades[0].quantity, 3u);

    auto remaining = engine.Book().GetLevelVolume(Side::Sell, 100.50);
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(*remaining, 4u);
}

TEST(MatchingEngineTest, CancelNewestPreventsSelfMatchAgainstOwnRestingOrder) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 42, Side::Sell, 100.50, 5, 1)));

    Order incoming = MakeOrder(2, 42, Side::Buy, 101.00, 5, 2, SelfTradePrevention::CancelNewest);
    auto result = engine.MatchLimitOrder(incoming);

    EXPECT_TRUE(result.trades.empty());
    EXPECT_EQ(result.remaining_quantity, 5u);

    auto volume = engine.Book().GetLevelVolume(Side::Sell, 100.50);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 5u);

    auto best_ask = engine.Book().GetBestAsk();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_ask, 100.50);
}

TEST(MatchingEngineTest, CancelNewestAllowsMatchAgainstDifferentParticipant) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 7, Side::Sell, 100.50, 5, 1)));

    Order incoming = MakeOrder(2, 42, Side::Buy, 101.00, 5, 2, SelfTradePrevention::CancelNewest);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].taker_order_id, 2u);
    EXPECT_EQ(result.trades[0].maker_order_id, 1u);
    EXPECT_EQ(result.trades[0].side, Side::Buy);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 100.50);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    EXPECT_EQ(result.remaining_quantity, 0u);
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(MatchingEngineTest, CancelNewestLeavesRestingOrderUnchanged) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 42, Side::Sell, 100.50, 8, 1)));

    Order incoming = MakeOrder(2, 42, Side::Buy, 101.00, 3, 2, SelfTradePrevention::CancelNewest);
    auto result = engine.MatchLimitOrder(incoming);

    EXPECT_TRUE(result.trades.empty());
    EXPECT_EQ(result.remaining_quantity, 3u);

    auto volume = engine.Book().GetLevelVolume(Side::Sell, 100.50);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 8u);
}

TEST(MatchingEngineTest, CancelNewestStopsBeforeDeeperMarketableLevels) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 42, Side::Sell, 100.50, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(2, 7, Side::Sell, 100.75, 5, 2)));

    Order incoming = MakeOrder(3, 42, Side::Buy, 101.00, 10, 3, SelfTradePrevention::CancelNewest);
    auto result = engine.MatchLimitOrder(incoming);

    EXPECT_TRUE(result.trades.empty());
    EXPECT_EQ(result.remaining_quantity, 10u);

    auto first_level = engine.Book().GetLevelVolume(Side::Sell, 100.50);
    ASSERT_TRUE(first_level.has_value());
    EXPECT_EQ(*first_level, 5u);

    auto second_level = engine.Book().GetLevelVolume(Side::Sell, 100.75);
    ASSERT_TRUE(second_level.has_value());
    EXPECT_EQ(*second_level, 5u);
}

TEST(MatchingEngineTest, CancelNewestOnSellPreventsSelfMatchAgainstBestBid) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 42, Side::Buy, 100.00, 6, 1)));

    Order incoming = MakeOrder(2, 42, Side::Sell, 99.50, 6, 2, SelfTradePrevention::CancelNewest);
    auto result = engine.MatchLimitOrder(incoming);

    EXPECT_TRUE(result.trades.empty());
    EXPECT_EQ(result.remaining_quantity, 6u);

    auto volume = engine.Book().GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 6u);

    auto best_bid = engine.Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 100.00);
}

TEST(MatchingEngineTest, CancelOldestRemovesOwnRestingOrderAndContinues) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 42, Side::Sell, 100.50, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(2, 7, Side::Sell, 100.75, 5, 2)));

    Order incoming = MakeOrder(3, 42, Side::Buy, 101.00, 5, 3, SelfTradePrevention::CancelOldest);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].taker_order_id, 3u);
    EXPECT_EQ(result.trades[0].maker_order_id, 2u);
    EXPECT_EQ(result.trades[0].side, Side::Buy);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 100.75);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    EXPECT_FALSE(engine.Book().GetLevelVolume(Side::Sell, 100.50).has_value());
    EXPECT_FALSE(engine.Book().GetLevelVolume(Side::Sell, 100.75).has_value());
    EXPECT_EQ(result.remaining_quantity, 0u);
}

TEST(MatchingEngineTest, CancelOldestOnSellRemovesOwnBestBidAndContinues) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 42, Side::Buy, 100.00, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(2, 7, Side::Buy, 99.75, 5, 2)));

    Order incoming = MakeOrder(3, 42, Side::Sell, 99.50, 5, 3, SelfTradePrevention::CancelOldest);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].taker_order_id, 3u);
    EXPECT_EQ(result.trades[0].maker_order_id, 2u);
    EXPECT_EQ(result.trades[0].side, Side::Sell);
    EXPECT_DOUBLE_EQ(result.trades[0].price, 99.75);
    EXPECT_EQ(result.trades[0].quantity, 5u);

    EXPECT_FALSE(engine.Book().GetLevelVolume(Side::Buy, 100.00).has_value());
    EXPECT_FALSE(engine.Book().GetLevelVolume(Side::Buy, 99.75).has_value());
    EXPECT_EQ(result.remaining_quantity, 0u);
}

TEST(MatchingEngineTest, CancelOldestCancelsOnlyRestingSelfOrderWhenNoOtherLiquidityExists) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 42, Side::Sell, 100.50, 5, 1)));

    Order incoming = MakeOrder(2, 42, Side::Buy, 101.00, 5, 2, SelfTradePrevention::CancelOldest);
    auto result = engine.MatchLimitOrder(incoming);

    EXPECT_TRUE(result.trades.empty());
    EXPECT_EQ(result.remaining_quantity, 0u);

    auto best_bid = engine.Book().GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 101.00);

    auto volume = engine.Book().GetLevelVolume(Side::Buy, 101.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 5u);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(MatchingEngineTest, ExactCrossingBuyFullyConsumesBestAskWithoutResting) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Sell, 100.50, 5, 1)));

    Order incoming = MakeOrder(2, 2, Side::Buy, 100.50, 5, 2);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].quantity, 5u);
    EXPECT_EQ(result.remaining_quantity, 0u);
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
}

TEST(MatchingEngineTest, ExactCrossingSellFullyConsumesBestBidWithoutResting) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 5, 1)));

    Order incoming = MakeOrder(2, 2, Side::Sell, 100.00, 5, 2);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].quantity, 5u);
    EXPECT_EQ(result.remaining_quantity, 0u);
    EXPECT_FALSE(engine.Book().GetBestBid().has_value());
    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
}

TEST(MatchingEngineTest, NonCrossingSellRestsInBook) {
    MatchingEngine engine;

    Order sell = MakeOrder(1, 1, Side::Sell, 100.50, 10, 1);
    auto result = engine.MatchLimitOrder(sell);

    EXPECT_TRUE(result.trades.empty());
    EXPECT_EQ(result.remaining_quantity, 0u);

    auto best_ask = engine.Book().GetBestAsk();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_ask, 100.50);
}

TEST(MatchingEngineTest, PartialCrossingSellRestsRemainingQuantityAtLimitPrice) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 4, 1)));

    Order incoming = MakeOrder(2, 2, Side::Sell, 99.50, 10, 2);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 1u);
    EXPECT_EQ(result.trades[0].quantity, 4u);

    auto best_ask = engine.Book().GetBestAsk();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_ask, 99.50);

    auto volume = engine.Book().GetLevelVolume(Side::Sell, 99.50);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 6u);
}

TEST(MatchingEngineTest, CancelOldestRemovesSelfAtBestLevelThenContinuesAcrossRemainingLiquidity) {
    MatchingEngine engine;

    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(1, 42, Side::Sell, 100.50, 5, 1)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(2, 7, Side::Sell, 100.50, 4, 2)));
    EXPECT_TRUE(engine.Book().AddOrder(MakeOrder(3, 8, Side::Sell, 100.75, 3, 3)));

    Order incoming = MakeOrder(4, 42, Side::Buy, 101.00, 7, 4, SelfTradePrevention::CancelOldest);
    auto result = engine.MatchLimitOrder(incoming);

    ASSERT_EQ(result.trades.size(), 2u);
    EXPECT_EQ(result.trades[0].maker_order_id, 2u);
    EXPECT_EQ(result.trades[0].quantity, 4u);
    EXPECT_EQ(result.trades[1].maker_order_id, 3u);
    EXPECT_EQ(result.trades[1].quantity, 3u);

    EXPECT_FALSE(engine.Book().GetBestAsk().has_value());
    EXPECT_EQ(result.remaining_quantity, 0u);
}