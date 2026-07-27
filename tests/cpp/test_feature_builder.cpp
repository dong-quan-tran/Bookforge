#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "features/FeatureBuilder.hpp"
#include "features/FeatureCsvWriter.hpp"
#include "features/OfiFeatureBuilder.hpp"
#include "features/RollingFeatureBuilder.hpp"
#include "snapshot/BookSnapshot.hpp"

namespace bookforge {
namespace {

BookSnapshot MakeSnapshot(std::uint64_t event_index, std::uint64_t ts_ns,
                          std::optional<double> best_bid, std::optional<double> best_ask,
                          std::optional<double> spread, std::optional<double> mid_price,
                          std::vector<DepthLevelSnapshot> bids,
                          std::vector<DepthLevelSnapshot> asks) {
    BookSnapshot s{};
    s.symbol = "TEST";
    s.replay_event_index = event_index;
    s.replay_timestamp_ns = ts_ns;
    s.total_events_seen = event_index;
    s.submitted_orders = event_index;
    s.best_bid = best_bid;
    s.best_ask = best_ask;
    s.spread = spread;
    s.mid_price = mid_price;
    s.bids = std::move(bids);
    s.asks = std::move(asks);
    return s;
}

} // namespace

TEST(FeatureBuilderTest, BuildsTopOfBookFieldsFromSnapshot) {
    const BookSnapshot snapshot = MakeSnapshot(10, 1000, 100.0, 101.0, 1.0, 100.5,
                                               {{100.0, 12}, {99.0, 8}}, {{101.0, 6}, {102.0, 5}});

    const auto rows = FeatureBuilder::BuildFromSnapshots({snapshot}, 2);
    ASSERT_EQ(rows.size(), 1u);

    const auto &row = rows[0];
    EXPECT_EQ(row.symbol, "TEST");
    EXPECT_EQ(row.replay_event_index, 10u);
    EXPECT_EQ(row.replay_timestamp_ns, 1000u);

    ASSERT_TRUE(row.best_bid.has_value());
    ASSERT_TRUE(row.best_ask.has_value());
    ASSERT_TRUE(row.spread.has_value());
    ASSERT_TRUE(row.mid_price.has_value());

    EXPECT_DOUBLE_EQ(*row.best_bid, 100.0);
    EXPECT_DOUBLE_EQ(*row.best_ask, 101.0);
    EXPECT_DOUBLE_EQ(*row.spread, 1.0);
    EXPECT_DOUBLE_EQ(*row.mid_price, 100.5);
}

TEST(FeatureBuilderTest, ComputesL1DepthImbalance) {
    const BookSnapshot snapshot =
        MakeSnapshot(1, 1, 100.0, 101.0, 1.0, 100.5, {{100.0, 15}}, {{101.0, 5}});

    const auto rows = FeatureBuilder::BuildFromSnapshots({snapshot}, 1);
    ASSERT_EQ(rows.size(), 1u);

    const auto &row = rows[0];
    ASSERT_TRUE(row.l1_bid_qty.has_value());
    ASSERT_TRUE(row.l1_ask_qty.has_value());
    ASSERT_TRUE(row.l1_depth_imbalance.has_value());

    EXPECT_DOUBLE_EQ(*row.l1_bid_qty, 15.0);
    EXPECT_DOUBLE_EQ(*row.l1_ask_qty, 5.0);
    EXPECT_DOUBLE_EQ(*row.l1_depth_imbalance, 0.5);
}

TEST(FeatureBuilderTest, ComputesMultiLevelDepthImbalance) {
    const BookSnapshot snapshot =
        MakeSnapshot(2, 2, 100.0, 101.0, 1.0, 100.5, {{100.0, 10}, {99.0, 20}, {98.0, 30}},
                     {{101.0, 15}, {102.0, 5}, {103.0, 10}});

    const auto rows = FeatureBuilder::BuildFromSnapshots({snapshot}, 2);
    ASSERT_EQ(rows.size(), 1u);

    const auto &row = rows[0];
    ASSERT_TRUE(row.lN_bid_qty_sum.has_value());
    ASSERT_TRUE(row.lN_ask_qty_sum.has_value());
    ASSERT_TRUE(row.lN_depth_imbalance.has_value());

    EXPECT_DOUBLE_EQ(*row.lN_bid_qty_sum, 30.0);
    EXPECT_DOUBLE_EQ(*row.lN_ask_qty_sum, 20.0);
    EXPECT_DOUBLE_EQ(*row.lN_depth_imbalance, 0.2);
}

TEST(FeatureBuilderTest, LeavesImbalanceEmptyOnOneSidedBook) {
    const BookSnapshot snapshot =
        MakeSnapshot(3, 3, 100.0, std::nullopt, std::nullopt, std::nullopt, {{100.0, 10}}, {});

    const auto rows = FeatureBuilder::BuildFromSnapshots({snapshot}, 3);
    ASSERT_EQ(rows.size(), 1u);

    const auto &row = rows[0];
    ASSERT_TRUE(row.l1_bid_qty.has_value());
    EXPECT_FALSE(row.l1_ask_qty.has_value());
    EXPECT_FALSE(row.l1_depth_imbalance.has_value());

    ASSERT_TRUE(row.lN_bid_qty_sum.has_value());
    EXPECT_FALSE(row.lN_ask_qty_sum.has_value());
    EXPECT_FALSE(row.lN_depth_imbalance.has_value());
}

TEST(FeatureBuilderTest, BuildsMultipleRowsInInputOrder) {
    const BookSnapshot s1 =
        MakeSnapshot(1, 100, 100.0, 101.0, 1.0, 100.5, {{100.0, 10}}, {{101.0, 8}});
    const BookSnapshot s2 =
        MakeSnapshot(2, 200, 101.0, 102.0, 1.0, 101.5, {{101.0, 11}}, {{102.0, 9}});

    const auto rows = FeatureBuilder::BuildFromSnapshots({s1, s2}, 1);
    ASSERT_EQ(rows.size(), 2u);

    EXPECT_EQ(rows[0].replay_event_index, 1u);
    EXPECT_EQ(rows[1].replay_event_index, 2u);
    ASSERT_TRUE(rows[0].mid_price.has_value());
    ASSERT_TRUE(rows[1].mid_price.has_value());
    EXPECT_DOUBLE_EQ(*rows[0].mid_price, 100.5);
    EXPECT_DOUBLE_EQ(*rows[1].mid_price, 101.5);
}

TEST(FeatureBuilderTest, CsvWriterProducesStableHeaderAndRows) {
    const BookSnapshot snapshot = MakeSnapshot(5, 500, 100.0, 101.0, 1.0, 100.5,
                                               {{100.0, 12}, {99.0, 8}}, {{101.0, 6}, {102.0, 5}});

    const auto rows = FeatureBuilder::BuildFromSnapshots({snapshot}, 2);

    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_features.csv";

    FeatureCsvWriter::Write(out_path.string(), rows);

    std::ifstream in(out_path);
    ASSERT_TRUE(in.is_open());

    std::string header;
    std::getline(in, header);
    EXPECT_EQ(header,
              "symbol,replay_event_index,replay_timestamp_ns,best_bid,best_ask,spread,mid_price,"
              "l1_bid_qty,l1_ask_qty,l1_depth_imbalance,lN_bid_qty_sum,lN_ask_qty_sum,"
              "lN_depth_imbalance,ofi_l1,ofi_lN,weighted_ofi_lN,rolling_mean_spread,"
              "rolling_mean_l1_total_depth,rolling_mean_lN_total_depth,rolling_mid_return,"
              "rolling_realized_mid_vol,rolling_mean_abs_ofi_l1,rolling_mean_abs_ofi_lN");

    std::string row;
    std::getline(in, row);
    EXPECT_FALSE(row.empty());
    EXPECT_NE(row.find("TEST"), std::string::npos);
    EXPECT_NE(row.find("100.5"), std::string::npos);
}

TEST(FeatureBuilderTest, OfiL1HandlesPriceAndSizeChanges) {
    using DL = DepthLevelSnapshot;

    BookSnapshot prev{};
    prev.symbol = "TEST";
    prev.replay_event_index = 1;
    prev.replay_timestamp_ns = 100;
    prev.bids = {DL{100.0, 10}};
    prev.asks = {DL{101.0, 8}};

    BookSnapshot curr{};
    curr.symbol = "TEST";
    curr.replay_event_index = 2;
    curr.replay_timestamp_ns = 200;
    curr.bids = {DL{101.0, 12}}; // bid price up, new volume
    curr.asks = {DL{101.0, 6}};  // ask price same, volume down

    auto rows = FeatureBuilder::BuildFromSnapshots({prev, curr}, 1);
    OfiFeatureBuilder::AddOfiFeatures(rows, {prev, curr}, 1);

    ASSERT_EQ(rows.size(), 2u);
    EXPECT_FALSE(rows[0].ofi_l1.has_value());

    ASSERT_TRUE(rows[1].ofi_l1.has_value());
    const double ofi_l1 = *rows[1].ofi_l1;

    // Bid OFI: price up => +v_curr = +12
    // Ask OFI: price same => v_prev - v_curr = 8 - 6 = +2
    // Total OFI = 14
    EXPECT_DOUBLE_EQ(ofi_l1, 14.0);
}

TEST(FeatureBuilderTest, OfiLNAggregatesAcrossLevels) {
    using DL = DepthLevelSnapshot;

    BookSnapshot prev{};
    prev.symbol = "TEST";
    prev.replay_event_index = 1;
    prev.replay_timestamp_ns = 100;
    prev.bids = {DL{100.0, 10}, DL{99.0, 5}};
    prev.asks = {DL{101.0, 8}, DL{102.0, 4}};

    BookSnapshot curr{};
    curr.symbol = "TEST";
    curr.replay_event_index = 2;
    curr.replay_timestamp_ns = 200;
    curr.bids = {DL{100.0, 12}, DL{99.0, 5}}; // L1 bid size up
    curr.asks = {DL{101.0, 6}, DL{102.0, 6}}; // L1 ask size down, L2 ask size up

    auto rows = FeatureBuilder::BuildFromSnapshots({prev, curr}, 2);
    OfiFeatureBuilder::AddOfiFeatures(rows, {prev, curr}, 2);

    ASSERT_TRUE(rows[1].ofi_lN.has_value());
    const double ofi_lN = *rows[1].ofi_lN;

    // Level 1 bid: v_curr - v_prev = 12 - 10 = +2
    // Level 1 ask: v_prev - v_curr = 8 - 6 = +2
    // Level 2 bid: unchanged => 0
    // Level 2 ask: v_prev - v_curr = 4 - 6 = -2
    // Total OFI_LN = 2 + 2 + 0 - 2 = 2
    EXPECT_DOUBLE_EQ(ofi_lN, 2.0);
}

TEST(FeatureBuilderTest, OfiHandlesSideAppearingAndDisappearing) {
    using DL = DepthLevelSnapshot;

    BookSnapshot prev{};
    prev.symbol = "TEST";
    prev.replay_event_index = 1;
    prev.replay_timestamp_ns = 100;
    prev.bids = {};
    prev.asks = {DL{101.0, 5}};

    BookSnapshot curr{};
    curr.symbol = "TEST";
    curr.replay_event_index = 2;
    curr.replay_timestamp_ns = 200;
    curr.bids = {DL{100.0, 7}};
    curr.asks = {};

    auto rows = FeatureBuilder::BuildFromSnapshots({prev, curr}, 1);
    OfiFeatureBuilder::AddOfiFeatures(rows, {prev, curr}, 1);

    ASSERT_TRUE(rows[1].ofi_l1.has_value());
    const double ofi_l1 = *rows[1].ofi_l1;

    // Bid side: appears => +v_curr = +7
    // Ask side: disappears => +v_prev (is_bid_side=false) = +5
    // Total OFI_L1 = 12
    EXPECT_DOUBLE_EQ(ofi_l1, 12.0);
}

TEST(FeatureBuilderTest, WeightedOfiFirstRowIsEmpty) {
    using DL = DepthLevelSnapshot;

    BookSnapshot prev{};
    prev.symbol = "TEST";
    prev.replay_event_index = 1;
    prev.replay_timestamp_ns = 100;
    prev.bids = {DL{100.0, 10}};
    prev.asks = {DL{101.0, 8}};

    BookSnapshot curr{};
    curr.symbol = "TEST";
    curr.replay_event_index = 2;
    curr.replay_timestamp_ns = 200;
    curr.bids = {DL{101.0, 12}};
    curr.asks = {DL{101.0, 6}};

    auto rows = FeatureBuilder::BuildFromSnapshots({prev, curr}, 1);
    OfiFeatureBuilder::AddOfiFeatures(rows, {prev, curr}, 1);

    EXPECT_FALSE(rows[0].weighted_ofi_lN.has_value());
    ASSERT_TRUE(rows[1].weighted_ofi_lN.has_value());
}

TEST(FeatureBuilderTest, WeightedOfiDiscountsDeeperLevels) {
    using DL = DepthLevelSnapshot;

    BookSnapshot prev{};
    prev.symbol = "TEST";
    prev.replay_event_index = 1;
    prev.replay_timestamp_ns = 100;
    prev.bids = {DL{100.0, 10}, DL{99.0, 10}};
    prev.asks = {DL{101.0, 10}, DL{102.0, 10}};

    BookSnapshot curr{};
    curr.symbol = "TEST";
    curr.replay_event_index = 2;
    curr.replay_timestamp_ns = 200;
    curr.bids = {DL{100.0, 12}, DL{99.0, 14}};
    curr.asks = {DL{101.0, 10}, DL{102.0, 10}};

    auto rows = FeatureBuilder::BuildFromSnapshots({prev, curr}, 2);
    OfiFeatureBuilder::AddOfiFeatures(rows, {prev, curr}, 2);

    ASSERT_TRUE(rows[1].ofi_lN.has_value());
    ASSERT_TRUE(rows[1].weighted_ofi_lN.has_value());

    // Unweighted:
    // L1 bid = +2, L2 bid = +4, asks unchanged => total 6
    EXPECT_DOUBLE_EQ(*rows[1].ofi_lN, 6.0);

    // Weighted:
    // L1 weight 1.0 => 1.0 * 2 = 2
    // L2 weight 0.5 => 0.5 * 4 = 2
    // Total = 4
    EXPECT_DOUBLE_EQ(*rows[1].weighted_ofi_lN, 4.0);
}

TEST(FeatureBuilderTest, WeightedOfiMatchesUnweightedWhenOnlyLevelOneChanges) {
    using DL = DepthLevelSnapshot;

    BookSnapshot prev{};
    prev.symbol = "TEST";
    prev.replay_event_index = 1;
    prev.replay_timestamp_ns = 100;
    prev.bids = {DL{100.0, 10}, DL{99.0, 7}};
    prev.asks = {DL{101.0, 8}, DL{102.0, 6}};

    BookSnapshot curr{};
    curr.symbol = "TEST";
    curr.replay_event_index = 2;
    curr.replay_timestamp_ns = 200;
    curr.bids = {DL{100.0, 13}, DL{99.0, 7}};
    curr.asks = {DL{101.0, 5}, DL{102.0, 6}};

    auto rows = FeatureBuilder::BuildFromSnapshots({prev, curr}, 2);
    OfiFeatureBuilder::AddOfiFeatures(rows, {prev, curr}, 2);

    ASSERT_TRUE(rows[1].ofi_lN.has_value());
    ASSERT_TRUE(rows[1].weighted_ofi_lN.has_value());

    // L1 bid = +3, L1 ask = +3, L2 unchanged => total 6
    EXPECT_DOUBLE_EQ(*rows[1].ofi_lN, 6.0);
    EXPECT_DOUBLE_EQ(*rows[1].weighted_ofi_lN, 6.0);
}

TEST(FeatureBuilderTest, RollingContextComputesMeanSpreadAndDepth) {
    std::vector<FeatureRow> rows(3);

    rows[0].spread = 1.0;
    rows[0].l1_bid_qty = 10.0;
    rows[0].l1_ask_qty = 20.0;
    rows[0].lN_bid_qty_sum = 30.0;
    rows[0].lN_ask_qty_sum = 40.0;

    rows[1].spread = 2.0;
    rows[1].l1_bid_qty = 20.0;
    rows[1].l1_ask_qty = 10.0;
    rows[1].lN_bid_qty_sum = 40.0;
    rows[1].lN_ask_qty_sum = 20.0;

    rows[2].spread = 3.0;
    rows[2].l1_bid_qty = 30.0;
    rows[2].l1_ask_qty = 30.0;
    rows[2].lN_bid_qty_sum = 50.0;
    rows[2].lN_ask_qty_sum = 50.0;

    RollingFeatureBuilder::AddRollingContextFeatures(rows, 2);

    ASSERT_TRUE(rows[2].rolling_mean_spread.has_value());
    EXPECT_DOUBLE_EQ(*rows[2].rolling_mean_spread, 2.5);

    ASSERT_TRUE(rows[2].rolling_mean_l1_total_depth.has_value());
    EXPECT_DOUBLE_EQ(*rows[2].rolling_mean_l1_total_depth, 45.0);

    ASSERT_TRUE(rows[2].rolling_mean_lN_total_depth.has_value());
    EXPECT_DOUBLE_EQ(*rows[2].rolling_mean_lN_total_depth, 80.0);
}
TEST(FeatureBuilderTest, RollingContextComputesMidReturnAndRealizedVol) {
    std::vector<FeatureRow> rows(3);

    rows[0].mid_price = 100.0;
    rows[1].mid_price = 101.0;
    rows[2].mid_price = 103.0;

    RollingFeatureBuilder::AddRollingContextFeatures(rows, 3);

    ASSERT_TRUE(rows[2].rolling_mid_return.has_value());
    EXPECT_NEAR(*rows[2].rolling_mid_return, 0.03, 1e-12);

    ASSERT_TRUE(rows[2].rolling_realized_mid_vol.has_value());

    const double r1 = (101.0 / 100.0) - 1.0;
    const double r2 = (103.0 / 101.0) - 1.0;
    const double expected_vol = std::sqrt(r1 * r1 + r2 * r2);

    EXPECT_NEAR(*rows[2].rolling_realized_mid_vol, expected_vol, 1e-12);
}
TEST(FeatureBuilderTest, RollingContextComputesMeanAbsoluteOfi) {
    std::vector<FeatureRow> rows(3);

    rows[0].ofi_l1 = -2.0;
    rows[0].ofi_lN = -4.0;

    rows[1].ofi_l1 = 6.0;
    rows[1].ofi_lN = 8.0;

    rows[2].ofi_l1 = -4.0;
    rows[2].ofi_lN = 2.0;

    RollingFeatureBuilder::AddRollingContextFeatures(rows, 2);

    ASSERT_TRUE(rows[2].rolling_mean_abs_ofi_l1.has_value());
    EXPECT_DOUBLE_EQ(*rows[2].rolling_mean_abs_ofi_l1, 5.0);

    ASSERT_TRUE(rows[2].rolling_mean_abs_ofi_lN.has_value());
    EXPECT_DOUBLE_EQ(*rows[2].rolling_mean_abs_ofi_lN, 5.0);
}
TEST(FeatureBuilderTest, RollingContextRejectsZeroWindow) {
    std::vector<FeatureRow> rows(1);
    EXPECT_THROW(RollingFeatureBuilder::AddRollingContextFeatures(rows, 0), std::runtime_error);
}

} // namespace bookforge