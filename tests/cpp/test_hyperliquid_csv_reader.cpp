#include <gtest/gtest.h>

#include "HyperliquidCsvReader.hpp"

#include <fstream>
#include <string>

using namespace bookforge;

namespace {

std::string WriteTempCsv(const std::string &filename, const std::string &content) {
    std::ofstream out(filename, std::ios::trunc);
    out << content;
    out.close();
    return filename;
}

} // namespace

TEST(HyperliquidCsvReaderTest, ReadsValidRows) {
    const std::string path = WriteTempCsv(
        "hyperliquid_reader_valid_test.csv",
        "ts,limitPx,sz,isAsk,statusId,status,eventType\n"
        "2025-12-15 11:39:39.722049503,89691.0,0.01672,True,3,perpMarginRejected,Reject\n"
        "2025-12-15 11:39:39.722049504,89690.0,0.02000,False,1,open,New\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all();

    ASSERT_EQ(events.size(), 2u);

    EXPECT_DOUBLE_EQ(events[0].price, 89691.0);
    EXPECT_DOUBLE_EQ(events[0].size, 0.01672);
    EXPECT_TRUE(events[0].isAsk);
    EXPECT_EQ(events[0].statusId, 3);
    EXPECT_EQ(events[0].statusText, "perpMarginRejected");
    EXPECT_EQ(events[0].eventType, EventType::Reject);

    EXPECT_DOUBLE_EQ(events[1].price, 89690.0);
    EXPECT_DOUBLE_EQ(events[1].size, 0.02000);
    EXPECT_FALSE(events[1].isAsk);
    EXPECT_EQ(events[1].statusId, 1);
    EXPECT_EQ(events[1].statusText, "open");
    EXPECT_EQ(events[1].eventType, EventType::New);
}

TEST(HyperliquidCsvReaderTest, SkipsMalformedRowsInNonStrictMode) {
    const std::string path = WriteTempCsv(
        "hyperliquid_reader_nonstrict_test.csv",
        "ts,limitPx,sz,isAsk,statusId,status,eventType\n"
        "2025-12-15 11:39:39.722049503,89691.0,0.01672,True,3,perpMarginRejected,Reject\n"
        "bad,row\n"
        "2025-12-15 11:39:39.722049504,89690.0,0.02000,False,1,open,New\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all(false, false);

    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].eventType, EventType::Reject);
    EXPECT_EQ(events[1].eventType, EventType::New);
}

TEST(HyperliquidCsvReaderTest, ThrowsOnMalformedRowsInStrictMode) {
    const std::string path = WriteTempCsv(
        "hyperliquid_reader_strict_test.csv",
        "ts,limitPx,sz,isAsk,statusId,status,eventType\n"
        "2025-12-15 11:39:39.722049503,89691.0,0.01672,True,3,perpMarginRejected,Reject\n"
        "bad,row\n");

    HyperliquidCsvReader reader(path);

    EXPECT_THROW(
        {
            const auto events = reader.read_all(true, false);
            (void)events;
        },
        std::exception);
}

TEST(HyperliquidCsvReaderTest, MapsUnknownStatusToOther) {
    const std::string path =
        WriteTempCsv("hyperliquid_reader_other_test.csv",
                     "ts,limitPx,sz,isAsk,statusId,status,eventType\n"
                     "2025-12-15 11:39:39.722049503,89691.0,0.01672,True,9,mysteryState,Other\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all();

    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].eventType, EventType::Other);
}