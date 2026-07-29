#include <gtest/gtest.h>

#include "core/order_book.hpp"

using namespace bookforge;

namespace {

Order MakeOrder(std::uint64_t id, std::uint64_t participant_id, Side side, double price,
                std::uint32_t quantity, std::uint64_t timestamp,
                SelfTradePrevention stp = SelfTradePrevention::None) {
    return Order{id, participant_id, side, price, quantity, timestamp, stp};
}

} // namespace

TEST(OrderBookTest, EmptyBookHasNoBestBidOrAsk) {
    OrderBook book;

    EXPECT_FALSE(book.GetBestBid().has_value());
    EXPECT_FALSE(book.GetBestAsk().has_value());
    EXPECT_FALSE(book.GetMidPrice().has_value());
    EXPECT_FALSE(book.GetSpread().has_value());
}

TEST(OrderBookTest, AddSingleBid) {
    OrderBook book;
    Order order = MakeOrder(1, 1, Side::Buy, 100.25, 10, 1);

    EXPECT_TRUE(book.AddOrder(order));

    ASSERT_TRUE(book.GetBestBid().has_value());
    EXPECT_DOUBLE_EQ(*book.GetBestBid(), 100.25);
    EXPECT_EQ(book.BidLevelCount(), 1);
}

TEST(OrderBookTest, AddBidAndAskComputesMidAndSpread) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Sell, 100.50, 12, 2)));

    ASSERT_TRUE(book.GetBestBid().has_value());
    ASSERT_TRUE(book.GetBestAsk().has_value());
    ASSERT_TRUE(book.GetMidPrice().has_value());
    ASSERT_TRUE(book.GetSpread().has_value());

    EXPECT_DOUBLE_EQ(*book.GetBestBid(), 100.00);
    EXPECT_DOUBLE_EQ(*book.GetBestAsk(), 100.50);
    EXPECT_DOUBLE_EQ(*book.GetMidPrice(), 100.25);
    EXPECT_DOUBLE_EQ(*book.GetSpread(), 0.50);
}

TEST(OrderBookTest, CancelOrderRemovesLevelWhenEmpty) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 99.50, 20, 1)));

    EXPECT_TRUE(book.CancelOrder(1));
    EXPECT_EQ(book.BidLevelCount(), 0);
    EXPECT_FALSE(book.GetBestBid().has_value());
}

TEST(OrderBookTest, MultipleOrdersSameLevelAggregateVolume) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 100.00, 15, 2)));

    auto volume = book.GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 25);
}

TEST(OrderBookTest, BestBidTracksHighestPrice) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 99.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 101.00, 10, 2)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(3, 3, Side::Buy, 100.00, 10, 3)));

    ASSERT_TRUE(book.GetBestBid().has_value());
    EXPECT_DOUBLE_EQ(*book.GetBestBid(), 101.00);
}

TEST(OrderBookTest, BestAskTracksLowestPrice) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 102.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Sell, 100.50, 10, 2)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(3, 3, Side::Sell, 101.00, 10, 3)));

    ASSERT_TRUE(book.GetBestAsk().has_value());
    EXPECT_DOUBLE_EQ(*book.GetBestAsk(), 100.50);
}

TEST(OrderBookTest, BidDepthReturnsSortedLevels) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 99.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 101.00, 20, 2)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(3, 3, Side::Buy, 100.00, 30, 3)));

    auto depth = book.GetBidDepth(2);
    ASSERT_EQ(depth.size(), 2);
    EXPECT_DOUBLE_EQ(depth[0].first, 101.00);
    EXPECT_EQ(depth[0].second, 20);
    EXPECT_DOUBLE_EQ(depth[1].first, 100.00);
    EXPECT_EQ(depth[1].second, 30);
}

TEST(OrderBookTest, AskDepthReturnsSortedLevels) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 102.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Sell, 100.50, 20, 2)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(3, 3, Side::Sell, 101.00, 30, 3)));

    auto depth = book.GetAskDepth(2);
    ASSERT_EQ(depth.size(), 2);
    EXPECT_DOUBLE_EQ(depth[0].first, 100.50);
    EXPECT_EQ(depth[0].second, 20);
    EXPECT_DOUBLE_EQ(depth[1].first, 101.00);
    EXPECT_EQ(depth[1].second, 30);
}

TEST(OrderBookTest, PartialExecutionReducesFrontOrderVolume) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 20, 1)));

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Buy, 100.00, 5));

    auto volume = book.GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 15);
}

TEST(OrderBookTest, FullExecutionRemovesPriceLevel) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 101.00, 20, 1)));

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Sell, 101.00, 20));

    EXPECT_FALSE(book.GetLevelVolume(Side::Sell, 101.00).has_value());
    EXPECT_EQ(book.AskLevelCount(), 0);
}

TEST(OrderBookTest, CancelMissingOrderReturnsFalse) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));

    EXPECT_FALSE(book.CancelOrder(999));
}

TEST(OrderBookTest, ExecuteMissingLevelReturnsFalse) {
    OrderBook book;
    EXPECT_FALSE(book.ExecuteTopOrder(Side::Buy, 123.45, 10));
}

TEST(OrderBookTest, ExecuteTopOrderConsumesOldestOrderFirst) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 100.00, 15, 2)));

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Buy, 100.00, 10));

    auto volume = book.GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 15);

    EXPECT_FALSE(book.CancelOrder(1));
    EXPECT_TRUE(book.CancelOrder(2));
    EXPECT_FALSE(book.GetLevelVolume(Side::Buy, 100.00).has_value());
}

TEST(OrderBookTest, ExecuteTopOrderPartiallyConsumesOldestBeforeNextOrder) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 101.00, 20, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Sell, 101.00, 30, 2)));

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Sell, 101.00, 5));

    auto volume = book.GetLevelVolume(Side::Sell, 101.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 45);

    EXPECT_TRUE(book.CancelOrder(1));
    auto remaining = book.GetLevelVolume(Side::Sell, 101.00);
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(*remaining, 30);
}

TEST(OrderBookTest, ExecuteZeroQuantityLeavesVolumeUnchanged) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 20, 1)));

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Buy, 100.00, 0));

    auto volume = book.GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 20);
}

TEST(OrderBookTest, CancelOrderByIdRemovesAsk) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 101.50, 25, 1)));

    EXPECT_TRUE(book.CancelOrder(1));
    EXPECT_EQ(book.AskLevelCount(), 0);
    EXPECT_FALSE(book.GetBestAsk().has_value());
}

TEST(OrderBookTest, FullExecutionRemovesOrderFromIndex) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Buy, 100.00, 10));
    EXPECT_FALSE(book.CancelOrder(1));
}

TEST(OrderBookTest, BidDepthRequestLargerThanAvailableReturnsAllLevels) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 101.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 100.00, 20, 2)));

    auto depth = book.GetBidDepth(10);
    ASSERT_EQ(depth.size(), 2);
    EXPECT_DOUBLE_EQ(depth[0].first, 101.00);
    EXPECT_EQ(depth[0].second, 10);
    EXPECT_DOUBLE_EQ(depth[1].first, 100.00);
    EXPECT_EQ(depth[1].second, 20);
}

TEST(OrderBookTest, AskDepthRequestLargerThanAvailableReturnsAllLevels) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 100.50, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Sell, 101.50, 20, 2)));

    auto depth = book.GetAskDepth(10);
    ASSERT_EQ(depth.size(), 2);
    EXPECT_DOUBLE_EQ(depth[0].first, 100.50);
    EXPECT_EQ(depth[0].second, 10);
    EXPECT_DOUBLE_EQ(depth[1].first, 101.50);
    EXPECT_EQ(depth[1].second, 20);
}

TEST(OrderBookTest, MidPriceUnavailableWhenOnlyBidExists) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));

    EXPECT_FALSE(book.GetMidPrice().has_value());
}

TEST(OrderBookTest, SpreadUnavailableWhenOnlyAskExists) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 100.50, 10, 1)));

    EXPECT_FALSE(book.GetSpread().has_value());
}

TEST(OrderBookTest, AddDuplicateOrderIdReturnsFalse) {
    OrderBook book;

    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_FALSE(book.AddOrder(MakeOrder(1, 2, Side::Sell, 100.50, 20, 2)));
}

TEST(OrderBookTest, DuplicateOrderIdDoesNotChangeBookState) {
    OrderBook book;

    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_FALSE(book.AddOrder(MakeOrder(1, 2, Side::Buy, 101.00, 20, 2)));

    ASSERT_TRUE(book.GetBestBid().has_value());
    EXPECT_DOUBLE_EQ(*book.GetBestBid(), 100.00);
    EXPECT_EQ(book.BidLevelCount(), 1);

    auto volume = book.GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 10);

    EXPECT_FALSE(book.GetLevelVolume(Side::Buy, 101.00).has_value());
}

TEST(OrderBookTest, ReduceMissingOrderReturnsFalse) {
    OrderBook book;
    EXPECT_FALSE(book.ReduceOrderQuantity(999, 5));
}

TEST(OrderBookTest, ReduceOrderQuantityRejectsZero) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_FALSE(book.ReduceOrderQuantity(1, 0));
}

TEST(OrderBookTest, ReduceOrderQuantityRejectsNonReduction) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_FALSE(book.ReduceOrderQuantity(1, 10));
    EXPECT_FALSE(book.ReduceOrderQuantity(1, 12));
}

TEST(OrderBookTest, ReduceOrderQuantityKeepsPriority) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 100.00, 20, 2)));

    EXPECT_TRUE(book.ReduceOrderQuantity(1, 6));

    auto volume = book.GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 26);

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Buy, 100.00, 6));
    EXPECT_FALSE(book.CancelOrder(1));
    EXPECT_TRUE(book.CancelOrder(2));
}

TEST(OrderBookTest, ReplaceMissingOrderReturnsFalse) {
    OrderBook book;
    EXPECT_FALSE(book.ReplaceOrder(999, 101.00, 5, 10));
}

TEST(OrderBookTest, ReplaceOrderMovesToNewPriceLevel) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));

    EXPECT_TRUE(book.ReplaceOrder(1, 101.00, 7, 2));

    EXPECT_FALSE(book.GetLevelVolume(Side::Buy, 100.00).has_value());

    auto volume = book.GetLevelVolume(Side::Buy, 101.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 7);

    auto best_bid = book.GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 101.00);
}

TEST(OrderBookTest, ReplaceOrderLosesPriorityAtSamePrice) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 100.00, 20, 2)));

    EXPECT_TRUE(book.ReplaceOrder(1, 100.00, 10, 3));

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Buy, 100.00, 20));
    EXPECT_FALSE(book.CancelOrder(2));
    EXPECT_TRUE(book.CancelOrder(1));
}

TEST(OrderBookTest, ReplaceOrderLosesPriorityAtNewPrice) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 101.00, 20, 2)));

    EXPECT_TRUE(book.ReplaceOrder(1, 101.00, 10, 3));

    auto volume = book.GetLevelVolume(Side::Buy, 101.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 30);

    EXPECT_TRUE(book.ExecuteTopOrder(Side::Buy, 101.00, 20));
    EXPECT_FALSE(book.CancelOrder(2));
    EXPECT_TRUE(book.CancelOrder(1));
}

TEST(OrderBookTest, CancelOneOfTwoOrdersAtSameLevelKeepsLevelWithRemainingVolume) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 100.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 100.00, 15, 2)));

    EXPECT_TRUE(book.CancelOrder(1));

    auto volume = book.GetLevelVolume(Side::Buy, 100.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 15);

    auto best_bid = book.GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 100.00);
}

TEST(OrderBookTest, ReduceOrderQuantityToOneUpdatesAggregateVolume) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 101.00, 10, 1)));
    EXPECT_TRUE(book.ReduceOrderQuantity(1, 1));

    auto volume = book.GetLevelVolume(Side::Sell, 101.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 1);
}

TEST(OrderBookTest, ReplaceOrderSamePriceUpdatesQuantityAndKeepsLevel) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 101.00, 10, 1)));

    EXPECT_TRUE(book.ReplaceOrder(1, 101.00, 6, 2));

    auto volume = book.GetLevelVolume(Side::Sell, 101.00);
    ASSERT_TRUE(volume.has_value());
    EXPECT_EQ(*volume, 6);

    auto best_ask = book.GetBestAsk();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_ask, 101.00);
}

TEST(OrderBookTest, ReplaceOrderToWorseBidUpdatesBestBid) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Buy, 101.00, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Buy, 100.00, 20, 2)));

    EXPECT_TRUE(book.ReplaceOrder(1, 99.50, 10, 3));

    auto best_bid = book.GetBestBid();
    ASSERT_TRUE(best_bid.has_value());
    EXPECT_DOUBLE_EQ(*best_bid, 100.00);
}

TEST(OrderBookTest, ReplaceOrderToWorseAskUpdatesBestAsk) {
    OrderBook book;
    EXPECT_TRUE(book.AddOrder(MakeOrder(1, 1, Side::Sell, 100.50, 10, 1)));
    EXPECT_TRUE(book.AddOrder(MakeOrder(2, 2, Side::Sell, 101.00, 20, 2)));

    EXPECT_TRUE(book.ReplaceOrder(1, 101.50, 10, 3));

    auto best_ask = book.GetBestAsk();
    ASSERT_TRUE(best_ask.has_value());
    EXPECT_DOUBLE_EQ(*best_ask, 101.00);
}