#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "core/matching_engine.hpp"
#include "core/order.hpp"
#include "snapshot/BookSnapshot.hpp"
#include "snapshot/SnapshotBinaryDeserializer.hpp"
#include "snapshot/SnapshotBinarySerializer.hpp"
#include "snapshot/SnapshotBuilder.hpp"
#include "snapshot/SnapshotComparator.hpp"
#include "snapshot/SnapshotDeserializer.hpp"
#include "snapshot/SnapshotSerializer.hpp"

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

SnapshotBuildContext MakeContext(std::uint64_t event_index, std::uint64_t ts) {
    SnapshotBuildContext ctx{};
    ctx.symbol = "TEST";
    ctx.replay_event_index = event_index;
    ctx.replay_timestamp_ns = ts;
    ctx.total_events_seen = event_index;
    ctx.submitted_orders = event_index;
    ctx.rejected_events = 0;
    ctx.ignored_events = 0;
    ctx.generated_trades = 0;
    return ctx;
}

TEST(SnapshotTest, BuilderCapturesTopOfBookFields) {
    MatchingEngine engine;
    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));
    engine.MatchLimitOrder(MakeOrder(2, Side::Sell, 102.0, 8, 2));

    const BookSnapshot snapshot = SnapshotBuilder::Build(engine, MakeContext(2, 2), 2);

    ASSERT_TRUE(snapshot.best_bid.has_value());
    ASSERT_TRUE(snapshot.best_ask.has_value());
    ASSERT_TRUE(snapshot.mid_price.has_value());
    ASSERT_TRUE(snapshot.spread.has_value());

    EXPECT_DOUBLE_EQ(*snapshot.best_bid, 100.0);
    EXPECT_DOUBLE_EQ(*snapshot.best_ask, 102.0);
    EXPECT_DOUBLE_EQ(*snapshot.mid_price, 101.0);
    EXPECT_DOUBLE_EQ(*snapshot.spread, 2.0);
}

TEST(SnapshotTest, BuilderExportsTopNDepth) {
    MatchingEngine engine;
    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));
    engine.MatchLimitOrder(MakeOrder(2, Side::Buy, 99.0, 7, 2));
    engine.MatchLimitOrder(MakeOrder(3, Side::Buy, 98.0, 5, 3));
    engine.MatchLimitOrder(MakeOrder(4, Side::Sell, 101.0, 8, 4));
    engine.MatchLimitOrder(MakeOrder(5, Side::Sell, 102.0, 6, 5));

    const BookSnapshot snapshot = SnapshotBuilder::Build(engine, MakeContext(5, 5), 2);

    ASSERT_EQ(snapshot.bids.size(), 2u);
    ASSERT_EQ(snapshot.asks.size(), 2u);

    EXPECT_DOUBLE_EQ(snapshot.bids[0].price, 100.0);
    EXPECT_EQ(snapshot.bids[0].quantity, 10u);
    EXPECT_DOUBLE_EQ(snapshot.bids[1].price, 99.0);
    EXPECT_EQ(snapshot.bids[1].quantity, 7u);

    EXPECT_DOUBLE_EQ(snapshot.asks[0].price, 101.0);
    EXPECT_EQ(snapshot.asks[0].quantity, 8u);
    EXPECT_DOUBLE_EQ(snapshot.asks[1].price, 102.0);
    EXPECT_EQ(snapshot.asks[1].quantity, 6u);
}

TEST(SnapshotTest, SerializerWritesStableCsvShape) {
    MatchingEngine engine;
    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));
    engine.MatchLimitOrder(MakeOrder(2, Side::Sell, 101.0, 4, 2));

    const BookSnapshot snapshot = SnapshotBuilder::Build(engine, MakeContext(2, 2), 2);

    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_snapshot_test.csv";

    SnapshotSerializer::WriteCsv(out_path.string(), {snapshot}, 2);

    std::ifstream in(out_path);
    ASSERT_TRUE(in.is_open());

    std::string header;
    std::string row;
    ASSERT_TRUE(static_cast<bool>(std::getline(in, header)));
    ASSERT_TRUE(static_cast<bool>(std::getline(in, row)));

    EXPECT_NE(header.find("symbol"), std::string::npos);
    EXPECT_NE(header.find("replay_event_index"), std::string::npos);
    EXPECT_NE(header.find("bid_px_1"), std::string::npos);
    EXPECT_NE(header.find("ask_px_2"), std::string::npos);

    EXPECT_NE(row.find("TEST"), std::string::npos);
    EXPECT_NE(row.find("100"), std::string::npos);
    EXPECT_NE(row.find("101"), std::string::npos);
}

TEST(SnapshotTest, ComparatorReportsEqualityForIdenticalSnapshots) {
    MatchingEngine engine;
    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));
    engine.MatchLimitOrder(MakeOrder(2, Side::Sell, 102.0, 8, 2));

    const BookSnapshot a = SnapshotBuilder::Build(engine, MakeContext(2, 2), 2);
    const BookSnapshot b = SnapshotBuilder::Build(engine, MakeContext(2, 2), 2);

    const auto result = SnapshotComparator::Compare(a, b);
    EXPECT_TRUE(result.equal);
    EXPECT_TRUE(result.message.empty());
}

TEST(SnapshotTest, ComparatorReportsFieldMismatch) {
    MatchingEngine engine;
    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));
    engine.MatchLimitOrder(MakeOrder(2, Side::Sell, 102.0, 8, 2));

    BookSnapshot a = SnapshotBuilder::Build(engine, MakeContext(2, 2), 2);
    BookSnapshot b = a;
    b.best_ask = 103.0;

    const auto result = SnapshotComparator::Compare(a, b);
    EXPECT_FALSE(result.equal);
    EXPECT_NE(result.message.find("best_ask"), std::string::npos);
}

TEST(SnapshotTest, ReplayStyleCheckpointValidationMatchesExpectedState) {
    MatchingEngine engine;
    std::vector<BookSnapshot> snapshots;

    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 5, 1));
    snapshots.push_back(SnapshotBuilder::Build(engine, MakeContext(1, 1), 2));

    engine.MatchLimitOrder(MakeOrder(2, Side::Sell, 101.0, 7, 2));
    snapshots.push_back(SnapshotBuilder::Build(engine, MakeContext(2, 2), 2));

    engine.MatchLimitOrder(MakeOrder(3, Side::Buy, 101.0, 3, 3));
    auto ctx = MakeContext(3, 3);
    ctx.generated_trades = 1;
    snapshots.push_back(SnapshotBuilder::Build(engine, ctx, 2));

    ASSERT_EQ(snapshots.size(), 3u);

    BookSnapshot expected{};
    expected.symbol = "TEST";
    expected.replay_event_index = 3;
    expected.replay_timestamp_ns = 3;
    expected.total_events_seen = 3;
    expected.submitted_orders = 3;
    expected.rejected_events = 0;
    expected.ignored_events = 0;
    expected.generated_trades = 1;
    expected.best_bid = 100.0;
    expected.best_ask = 101.0;
    expected.mid_price = 100.5;
    expected.spread = 1.0;
    expected.bids = {DepthLevelSnapshot{100.0, 5}};
    expected.asks = {DepthLevelSnapshot{101.0, 4}};

    const auto result = SnapshotComparator::Compare(snapshots.back(), expected);
    EXPECT_TRUE(result.equal) << result.message;
}

TEST(SnapshotTest, CsvRoundTripPreservesSnapshotContent) {
    MatchingEngine engine;
    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));
    engine.MatchLimitOrder(MakeOrder(2, Side::Buy, 99.0, 7, 2));
    engine.MatchLimitOrder(MakeOrder(3, Side::Sell, 101.0, 4, 3));

    SnapshotBuildContext ctx{};
    ctx.symbol = "TEST";
    ctx.replay_event_index = 3;
    ctx.replay_timestamp_ns = 3;
    ctx.total_events_seen = 3;
    ctx.submitted_orders = 3;
    ctx.rejected_events = 0;
    ctx.ignored_events = 0;
    ctx.generated_trades = 0;

    const BookSnapshot original = SnapshotBuilder::Build(engine, ctx, 2);

    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_snapshot_roundtrip.csv";

    SnapshotSerializer::WriteCsv(out_path.string(), {original}, 2);

    const auto loaded = SnapshotDeserializer::ReadCsv(out_path.string(), 2);
    ASSERT_EQ(loaded.size(), 1u);

    const auto result = SnapshotComparator::Compare(original, loaded[0]);
    EXPECT_TRUE(result.equal) << result.message;
}

TEST(SnapshotTest, DeserializerRejectsInvalidHeader) {
    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_snapshot_bad_header.csv";

    {
        std::ofstream out(out_path);
        ASSERT_TRUE(out.is_open());
        out << "wrong,header\n";
        out << "TEST,1,1,1,1,0,0,0,100,101,100.5,1,100,10,99,7,101,4,102,6\n";
    }

    EXPECT_THROW((void)SnapshotDeserializer::ReadCsv(out_path.string(), 2), std::runtime_error);
}

TEST(SnapshotTest, DeserializerPreservesEmptyOptionalTopOfBookFields) {
    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_snapshot_empty_optional.csv";

    {
        std::ofstream out(out_path);
        ASSERT_TRUE(out.is_open());
        out << "symbol,replay_event_index,replay_timestamp_ns,total_events_seen,submitted_orders,"
               "rejected_events,ignored_events,generated_trades,best_bid,best_ask,mid_price,spread,"
               "bid_px_1,bid_qty_1,ask_px_1,ask_qty_1\n";
        out << "TEST,1,1,1,1,0,0,0,,,,,100,5,101,6\n";
    }

    const auto loaded = SnapshotDeserializer::ReadCsv(out_path.string(), 1);
    ASSERT_EQ(loaded.size(), 1u);

    EXPECT_FALSE(loaded[0].best_bid.has_value());
    EXPECT_FALSE(loaded[0].best_ask.has_value());
    EXPECT_FALSE(loaded[0].mid_price.has_value());
    EXPECT_FALSE(loaded[0].spread.has_value());

    ASSERT_EQ(loaded[0].bids.size(), 1u);
    ASSERT_EQ(loaded[0].asks.size(), 1u);
    EXPECT_DOUBLE_EQ(loaded[0].bids[0].price, 100.0);
    EXPECT_EQ(loaded[0].bids[0].quantity, 5u);
    EXPECT_DOUBLE_EQ(loaded[0].asks[0].price, 101.0);
    EXPECT_EQ(loaded[0].asks[0].quantity, 6u);
}

TEST(SnapshotTest, BinaryRoundTripPreservesSnapshotContent) {
    MatchingEngine engine;
    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));
    engine.MatchLimitOrder(MakeOrder(2, Side::Buy, 99.0, 7, 2));
    engine.MatchLimitOrder(MakeOrder(3, Side::Sell, 101.0, 4, 3));

    SnapshotBuildContext ctx{};
    ctx.symbol = "TEST";
    ctx.replay_event_index = 3;
    ctx.replay_timestamp_ns = 3;
    ctx.total_events_seen = 3;
    ctx.submitted_orders = 3;
    ctx.rejected_events = 0;
    ctx.ignored_events = 0;
    ctx.generated_trades = 0;

    const BookSnapshot original = SnapshotBuilder::Build(engine, ctx, 2);

    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_snapshot_roundtrip.bin";

    SnapshotBinarySerializer::Write(out_path.string(), {original}, 2);

    const auto loaded = SnapshotBinaryDeserializer::Read(out_path.string(), 2);
    ASSERT_EQ(loaded.size(), 1u);

    const auto result = SnapshotComparator::Compare(original, loaded[0]);
    EXPECT_TRUE(result.equal) << result.message;
}

TEST(SnapshotTest, BinaryDeserializerRejectsInvalidMagic) {
    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_snapshot_bad_magic.bin";

    {
        std::ofstream out(out_path, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        const char bad_magic[8] = {'B', 'A', 'D', 'M', 'A', 'G', 'I', 'C'};
        out.write(bad_magic, sizeof(bad_magic));
    }

    EXPECT_THROW((void)SnapshotBinaryDeserializer::Read(out_path.string(), 2), std::runtime_error);
}

TEST(SnapshotTest, BinaryDeserializerRejectsUnsupportedVersion) {
    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_snapshot_bad_version.bin";

    {
        std::ofstream out(out_path, std::ios::binary);
        ASSERT_TRUE(out.is_open());

        const char magic[8] = {'B', 'F', 'S', 'N', 'A', 'P', '0', '1'};
        out.write(magic, sizeof(magic));

        const unsigned char version[4] = {2, 0, 0, 0};
        const unsigned char reserved[4] = {0, 0, 0, 0};
        const unsigned char depth[4] = {2, 0, 0, 0};
        const unsigned char count[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        out.write(reinterpret_cast<const char *>(version), sizeof(version));
        out.write(reinterpret_cast<const char *>(reserved), sizeof(reserved));
        out.write(reinterpret_cast<const char *>(depth), sizeof(depth));
        out.write(reinterpret_cast<const char *>(count), sizeof(count));
    }

    EXPECT_THROW((void)SnapshotBinaryDeserializer::Read(out_path.string(), 2), std::runtime_error);
}

TEST(SnapshotTest, BinaryDeserializerRejectsDepthMismatch) {
    MatchingEngine engine;
    engine.MatchLimitOrder(MakeOrder(1, Side::Buy, 100.0, 10, 1));

    SnapshotBuildContext ctx{};
    ctx.symbol = "TEST";
    ctx.replay_event_index = 1;
    ctx.replay_timestamp_ns = 1;
    ctx.total_events_seen = 1;
    ctx.submitted_orders = 1;
    ctx.rejected_events = 0;
    ctx.ignored_events = 0;
    ctx.generated_trades = 0;

    const BookSnapshot snapshot = SnapshotBuilder::Build(engine, ctx, 1);

    const std::filesystem::path out_path =
        std::filesystem::temp_directory_path() / "bookforge_snapshot_depth_mismatch.bin";

    SnapshotBinarySerializer::Write(out_path.string(), {snapshot}, 1);

    EXPECT_THROW((void)SnapshotBinaryDeserializer::Read(out_path.string(), 2), std::runtime_error);
}

} // namespace
} // namespace bookforge