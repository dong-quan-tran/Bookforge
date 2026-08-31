#include "HyperliquidCsvReader.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bookforge {
namespace {

constexpr char kUtf8BomFirstByte = static_cast<char>(0xEF);
constexpr char kUtf8BomSecondByte = static_cast<char>(0xBB);
constexpr char kUtf8BomThirdByte = static_cast<char>(0xBF);

std::string trim(const std::string &value) {
    const auto begin = std::find_if_not(value.begin(), value.end(),
                                        [](unsigned char ch) { return std::isspace(ch); });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
                         return std::isspace(ch);
                     }).base();

    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
}

void StripUtf8Bom(std::string &value) {
    if (value.size() < 3) {
        return;
    }

    if (value[0] == kUtf8BomFirstByte && value[1] == kUtf8BomSecondByte &&
        value[2] == kUtf8BomThirdByte) {
        value.erase(0, 3);
    }
}

std::vector<std::string> split_csv_simple(const std::string &line) {
    std::vector<std::string> fields;
    std::stringstream stream(line);
    std::string item;

    while (std::getline(stream, item, ',')) {
        fields.push_back(trim(item));
    }

    return fields;
}

bool parse_bool(const std::string &value) {
    if (value == "True" || value == "true" || value == "1") {
        return true;
    }

    if (value == "False" || value == "false" || value == "0") {
        return false;
    }

    throw std::runtime_error("invalid boolean value: " + value);
}

bool IsLeapYear(int year_value) {
    return year_value % 4 == 0 && (year_value % 100 != 0 || year_value % 400 == 0);
}

int DaysInMonth(int year_value, int month_value) {
    constexpr std::array<int, 12> kDaysPerMonth{
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
    };

    if (month_value < 1 || month_value > 12) {
        return 0;
    }

    if (month_value == 2 && IsLeapYear(year_value)) {
        return 29;
    }

    return kDaysPerMonth[static_cast<std::size_t>(month_value - 1)];
}

bool IsDigits(const std::string &value) {
    return !value.empty() && std::all_of(value.begin(), value.end(),
                                         [](unsigned char ch) { return std::isdigit(ch) != 0; });
}

int ParseFixedWidthInteger(const std::string &value, std::size_t offset, std::size_t width,
                           const char *field_name) {
    if (offset + width > value.size()) {
        throw std::runtime_error(std::string("invalid timestamp ") + field_name);
    }

    const std::string field = value.substr(offset, width);
    if (!IsDigits(field)) {
        throw std::runtime_error(std::string("invalid timestamp ") + field_name);
    }

    return std::stoi(field);
}

std::chrono::nanoseconds ParseTimestampUtc(const std::string &value) {
    constexpr std::size_t kWholeSecondLength = 19;

    if (value.size() < kWholeSecondLength || value[4] != '-' || value[7] != '-' ||
        value[10] != ' ' || value[13] != ':' || value[16] != ':') {
        throw std::runtime_error("invalid timestamp format");
    }

    const int year_value = ParseFixedWidthInteger(value, 0, 4, "year");
    const int month_value = ParseFixedWidthInteger(value, 5, 2, "month");
    const int day_value = ParseFixedWidthInteger(value, 8, 2, "day");
    const int hour_value = ParseFixedWidthInteger(value, 11, 2, "hour");
    const int minute_value = ParseFixedWidthInteger(value, 14, 2, "minute");
    const int second_value = ParseFixedWidthInteger(value, 17, 2, "second");

    if (month_value < 1 || month_value > 12 || day_value < 1 ||
        day_value > DaysInMonth(year_value, month_value) || hour_value < 0 || hour_value > 23 ||
        minute_value < 0 || minute_value > 59 || second_value < 0 || second_value > 59) {
        throw std::runtime_error("timestamp value out of range");
    }

    std::int64_t fractional_nanoseconds = 0;

    if (value.size() > kWholeSecondLength) {
        if (value[kWholeSecondLength] != '.') {
            throw std::runtime_error("invalid timestamp fractional separator");
        }

        const std::string fraction = value.substr(kWholeSecondLength + 1);
        if (fraction.empty() || fraction.size() > 9 || !IsDigits(fraction)) {
            throw std::runtime_error("invalid timestamp fractional seconds");
        }

        fractional_nanoseconds = std::stoll(fraction);

        for (std::size_t i = fraction.size(); i < 9; ++i) {
            fractional_nanoseconds *= 10;
        }
    }

    using namespace std::chrono;

    const sys_days date = std::chrono::year{year_value} /
                          std::chrono::month{static_cast<unsigned>(month_value)} /
                          std::chrono::day{static_cast<unsigned>(day_value)};
    const sys_time<seconds> whole_seconds =
        date + hours{hour_value} + minutes{minute_value} + seconds{second_value};
    const auto epoch_nanoseconds =
        duration_cast<nanoseconds>(whole_seconds.time_since_epoch()).count();

    return nanoseconds{epoch_nanoseconds + fractional_nanoseconds};
}

std::unordered_map<std::string, std::size_t>
BuildHeaderIndex(const std::vector<std::string> &header_fields) {
    std::unordered_map<std::string, std::size_t> index;

    for (std::size_t i = 0; i < header_fields.size(); ++i) {
        index.emplace(header_fields[i], i);
    }

    return index;
}

bool HasHeaderField(const std::unordered_map<std::string, std::size_t> &header_index,
                    const std::string &field_name) {
    return header_index.find(field_name) != header_index.end();
}

const std::string &
GetRequiredField(const std::vector<std::string> &fields,
                 const std::unordered_map<std::string, std::size_t> &header_index,
                 const std::string &field_name) {
    const auto index_it = header_index.find(field_name);
    if (index_it == header_index.end()) {
        throw std::runtime_error("missing required CSV column: " + field_name);
    }

    if (index_it->second >= fields.size()) {
        throw std::runtime_error("missing CSV field value for column: " + field_name);
    }

    return fields[index_it->second];
}

const std::string &
GetOptionalField(const std::vector<std::string> &fields,
                 const std::unordered_map<std::string, std::size_t> &header_index,
                 const std::string &field_name) {
    static const std::string empty;

    const auto index_it = header_index.find(field_name);
    if (index_it == header_index.end() || index_it->second >= fields.size()) {
        return empty;
    }

    return fields[index_it->second];
}

std::string ParseSymbol(const std::vector<std::string> &fields,
                        const std::unordered_map<std::string, std::size_t> &header_index) {
    const bool has_symbol_column = HasHeaderField(header_index, "symbol");
    const bool has_coin_column = HasHeaderField(header_index, "coin");

    if (!has_symbol_column && !has_coin_column) {
        return "";
    }

    const std::string &symbol = has_symbol_column ? GetOptionalField(fields, header_index, "symbol")
                                                  : GetOptionalField(fields, header_index, "coin");

    if (symbol.empty()) {
        throw std::runtime_error("symbol column is present but symbol value is empty");
    }

    return symbol;
}

std::string ParseExternalOrderId(const std::vector<std::string> &fields,
                                 const std::unordered_map<std::string, std::size_t> &header_index) {
    constexpr const char *kOrderIdColumn = "order_id";
    constexpr const char *kOrderIdCamelCaseColumn = "orderId";
    constexpr const char *kOidColumn = "oid";

    if (HasHeaderField(header_index, kOrderIdColumn)) {
        return GetOptionalField(fields, header_index, kOrderIdColumn);
    }

    if (HasHeaderField(header_index, kOrderIdCamelCaseColumn)) {
        return GetOptionalField(fields, header_index, kOrderIdCamelCaseColumn);
    }

    if (HasHeaderField(header_index, kOidColumn)) {
        return GetOptionalField(fields, header_index, kOidColumn);
    }

    return "";
}

std::optional<double>
ParseExternalFillSize(const std::vector<std::string> &fields,
                      const std::unordered_map<std::string, std::size_t> &header_index) {
    constexpr const char *kFillSizeColumn = "fill_size";
    constexpr const char *kFillSizeCamelCaseColumn = "fillSize";
    constexpr const char *kFillSizeShortColumn = "fillSz";

    const std::string *value = nullptr;

    if (HasHeaderField(header_index, kFillSizeColumn)) {
        value = &GetOptionalField(fields, header_index, kFillSizeColumn);
    } else if (HasHeaderField(header_index, kFillSizeCamelCaseColumn)) {
        value = &GetOptionalField(fields, header_index, kFillSizeCamelCaseColumn);
    } else if (HasHeaderField(header_index, kFillSizeShortColumn)) {
        value = &GetOptionalField(fields, header_index, kFillSizeShortColumn);
    }

    if (value == nullptr || value->empty()) {
        return std::nullopt;
    }

    return std::stod(*value);
}

} // namespace

HyperliquidCsvReader::HyperliquidCsvReader(std::string path) : path_(std::move(path)) {}

std::vector<ExternalOrderEvent> HyperliquidCsvReader::read_all() {
    return read_all(false, true);
}

std::vector<ExternalOrderEvent> HyperliquidCsvReader::read_all(bool strict_mode, bool log_errors) {
    std::ifstream file(path_);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open CSV file: " + path_);
    }

    std::vector<ExternalOrderEvent> events;
    std::string line;
    std::size_t line_number = 0;
    bool header_read = false;
    std::unordered_map<std::string, std::size_t> header_index;

    while (std::getline(file, line)) {
        ++line_number;

        if (trim(line).empty()) {
            continue;
        }

        if (!header_read) {
            header_read = true;

            auto header_fields = split_csv_simple(line);
            if (!header_fields.empty()) {
                StripUtf8Bom(header_fields.front());
            }

            header_index = BuildHeaderIndex(header_fields);

            if (HasHeaderField(header_index, "ts") && HasHeaderField(header_index, "limitPx")) {
                continue;
            }

            header_index.clear();
        }

        try {
            const auto fields = split_csv_simple(line);

            ExternalOrderEvent event{};

            if (header_index.empty()) {
                if (fields.size() < 5) {
                    throw std::runtime_error("expected at least 5 CSV fields");
                }

                event.ts = ParseTimestampUtc(fields[0]);
                event.symbol = "";
                event.external_order_id = "";
                event.external_fill_size = std::nullopt;
                event.price = std::stod(fields[1]);
                event.size = std::stod(fields[2]);
                event.isAsk = parse_bool(fields[3]);
                event.statusId = std::stoi(fields[4]);

                if (fields.size() >= 6) {
                    event.statusText = fields[5];
                    event.eventType = map_event_type(fields[5]);
                } else {
                    event.statusText = "";
                    event.eventType = map_event_type(std::to_string(event.statusId));
                }
            } else {
                event.ts = ParseTimestampUtc(GetRequiredField(fields, header_index, "ts"));
                event.symbol = ParseSymbol(fields, header_index);
                event.external_order_id = ParseExternalOrderId(fields, header_index);
                event.external_fill_size = ParseExternalFillSize(fields, header_index);
                event.price = std::stod(GetRequiredField(fields, header_index, "limitPx"));
                event.size = std::stod(GetRequiredField(fields, header_index, "sz"));
                event.isAsk = parse_bool(GetRequiredField(fields, header_index, "isAsk"));
                event.statusId = std::stoi(GetRequiredField(fields, header_index, "statusId"));

                const std::string &status_text = GetOptionalField(fields, header_index, "status");
                event.statusText = status_text;

                if (!status_text.empty()) {
                    event.eventType = map_event_type(status_text);
                } else {
                    event.eventType = map_event_type(std::to_string(event.statusId));
                }
            }

            events.push_back(std::move(event));
        } catch (const std::exception &exception) {
            if (log_errors) {
                std::cerr << "Malformed Hyperliquid CSV row at line " << line_number << ": "
                          << exception.what() << '\n';
            }

            if (strict_mode) {
                throw;
            }
        }
    }

    return events;
}

EventType HyperliquidCsvReader::map_event_type(const std::string &status_text) const {
    const std::string status = trim(status_text);

    if (status == "open" || status == "resting" || status == "received") {
        return EventType::New;
    }

    if (status == "canceled" || status == "cancelled") {
        return EventType::Cancel;
    }

    if (status == "filled" || status == "partiallyFilled" || status == "partialFill") {
        return EventType::Fill;
    }

    if (status == "replaced" || status == "replace" || status == "amended" || status == "amend") {
        return EventType::Replace;
    }

    if (status == "triggered" || status == "trigger") {
        return EventType::Trigger;
    }

    if (status.find("Rejected") != std::string::npos ||
        status.find("rejected") != std::string::npos) {
        return EventType::Reject;
    }

    if (status == "1") {
        return EventType::New;
    }

    if (status == "2") {
        return EventType::Cancel;
    }

    if (status == "3") {
        return EventType::Fill;
    }

    if (status == "4") {
        return EventType::Reject;
    }

    if (status == "5") {
        return EventType::Trigger;
    }

    if (status == "6") {
        return EventType::Replace;
    }

    return EventType::Other;
}

} // namespace bookforge
