#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "HyperliquidCsvReader.hpp"

using namespace bookforge;

namespace {

std::string WriteTempCsv(const std::string &filename, const std::string &content) {
    std::ofstream output(filename, std::ios::trunc);
    output << content;
    output.close();
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

    ASSERT_EQ(events.size(), 2U);

    EXPECT_TRUE(events[0].symbol.empty());
    EXPECT_DOUBLE_EQ(events[0].price, 89691.0);
    EXPECT_DOUBLE_EQ(events[0].size, 0.01672);
    EXPECT_TRUE(events[0].isAsk);
    EXPECT_EQ(events[0].statusId, 3);
    EXPECT_EQ(events[0].statusText, "perpMarginRejected");
    EXPECT_EQ(events[0].eventType, EventType::Reject);

    EXPECT_TRUE(events[1].symbol.empty());
    EXPECT_DOUBLE_EQ(events[1].price, 89690.0);
    EXPECT_DOUBLE_EQ(events[1].size, 0.02000);
    EXPECT_FALSE(events[1].isAsk);
    EXPECT_EQ(events[1].statusId, 1);
    EXPECT_EQ(events[1].statusText, "open");
    EXPECT_EQ(events[1].eventType, EventType::New);
}

TEST(HyperliquidCsvReaderTest, ReadsSymbolColumnWhenPresent) {
    const std::string path =
        WriteTempCsv("hyperliquid_reader_symbol_test.csv",
                     "ts,symbol,limitPx,sz,isAsk,statusId,status,eventType\n"
                     "2025-12-15 11:39:39.722049503,BTC,89691.0,0.01672,True,1,open,New\n"
                     "2025-12-15 11:39:39.722049504,ETH,3000.0,0.02000,False,1,open,New\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all();

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0].symbol, "BTC");
    EXPECT_EQ(events[1].symbol, "ETH");
    EXPECT_DOUBLE_EQ(events[0].price, 89691.0);
    EXPECT_DOUBLE_EQ(events[1].price, 3000.0);
}

TEST(HyperliquidCsvReaderTest, ReadsCoinColumnAsSymbolWhenPresent) {
    const std::string path =
        WriteTempCsv("hyperliquid_reader_coin_test.csv",
                     "ts,coin,limitPx,sz,isAsk,statusId,status,eventType\n"
                     "2025-12-15 11:39:39.722049503,SOL,150.0,0.01672,True,1,open,New\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].symbol, "SOL");
    EXPECT_DOUBLE_EQ(events[0].price, 150.0);
}

TEST(HyperliquidCsvReaderTest, SkipsRowsWithEmptyDeclaredSymbolInNonStrictMode) {
    const std::string path =
        WriteTempCsv("hyperliquid_reader_empty_symbol_nonstrict_test.csv",
                     "ts,symbol,limitPx,sz,isAsk,statusId,status,eventType\n"
                     "2025-12-15 11:39:39.722049503,,89691.0,0.01672,True,1,open,New\n"
                     "2025-12-15 11:39:39.722049504,BTC,89690.0,0.02000,False,1,open,New\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all(false, false);

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].symbol, "BTC");
}

TEST(HyperliquidCsvReaderTest, ThrowsOnEmptyDeclaredSymbolInStrictMode) {
    const std::string path =
        WriteTempCsv("hyperliquid_reader_empty_symbol_strict_test.csv",
                     "ts,symbol,limitPx,sz,isAsk,statusId,status,eventType\n"
                     "2025-12-15 11:39:39.722049503,,89691.0,0.01672,True,1,open,New\n");

    HyperliquidCsvReader reader(path);

    EXPECT_THROW(
        {
            const auto events = reader.read_all(true, false);
            (void)events;
        },
        std::exception);
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

    ASSERT_EQ(events.size(), 2U);
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

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(events[0].eventType, EventType::Other);
}

TEST(HyperliquidCsvReaderTest, ReadsEmptyFileWithHeaderOnly) {
    const std::string path = WriteTempCsv("hyperliquid_reader_header_only_test.csv",
                                          "ts,limitPx,sz,isAsk,statusId,status,eventType\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all();

    EXPECT_TRUE(events.empty());
}

TEST(HyperliquidCsvReaderTest, ReadsFalseIsAskCorrectly) {
    const std::string path =
        WriteTempCsv("hyperliquid_reader_false_isask_test.csv",
                     "ts,limitPx,sz,isAsk,statusId,status,eventType\n"
                     "2025-12-15 11:39:39.722049503,89690.0,0.02000,False,1,open,New\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_FALSE(events[0].isAsk);
    EXPECT_EQ(events[0].eventType, EventType::New);
}

TEST(HyperliquidCsvReaderTest, ReadsMultipleEventTypesInOrder) {
    const std::string path = WriteTempCsv(
        "hyperliquid_reader_multiple_types_test.csv",
        "ts,limitPx,sz,isAsk,statusId,status,eventType\n"
        "2025-12-15 11:39:39.722049501,100.50,0.01000,True,1,open,New\n"
        "2025-12-15 11:39:39.722049502,100.75,0.01000,True,3,perpMarginRejected,Reject\n"
        "2025-12-15 11:39:39.722049503,101.00,0.01000,False,9,mysteryState,Other\n");

    HyperliquidCsvReader reader(path);
    const auto events = reader.read_all(false, false);

    ASSERT_EQ(events.size(), 3U);
    EXPECT_EQ(events[0].eventType, EventType::New);
    EXPECT_EQ(events[1].eventType, EventType::Reject);
    EXPECT_EQ(events[2].eventType, EventType::Other);
}