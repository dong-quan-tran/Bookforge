#include "HyperliquidCsvReader.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace bookforge {
namespace {

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

std::chrono::nanoseconds parse_timestamp_stub(const std::string &) {
    return std::chrono::nanoseconds{0};
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

            const auto header_fields = split_csv_simple(line);
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

                event.ts = parse_timestamp_stub(fields[0]);
                event.price = std::stod(fields[1]);
                event.size = std::stod(fields[2]);
                event.isAsk = parse_bool(fields[3]);
                event.statusId = std::stoi(fields[4]);
                event.symbol = "";

                if (fields.size() >= 6) {
                    event.statusText = fields[5];
                    event.eventType = map_event_type(fields[5]);
                } else {
                    event.statusText = "";
                    event.eventType = map_event_type(std::to_string(event.statusId));
                }
            } else {
                event.ts = parse_timestamp_stub(GetRequiredField(fields, header_index, "ts"));
                event.symbol = ParseSymbol(fields, header_index);
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

    return EventType::Other;
}

} // namespace bookforge