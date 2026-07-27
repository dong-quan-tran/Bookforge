#include "HyperliquidCsvReader.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace bookforge {
namespace {

std::string trim(const std::string &s) {
    const auto begin =
        std::find_if_not(s.begin(), s.end(), [](unsigned char ch) { return std::isspace(ch); });
    const auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
                         return std::isspace(ch);
                     }).base();

    if (begin >= end) {
        return "";
    }
    return std::string(begin, end);
}

std::vector<std::string> split_csv_simple(const std::string &line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, ',')) {
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
    bool header_skipped = false;

    while (std::getline(file, line)) {
        ++line_number;

        if (trim(line).empty()) {
            continue;
        }

        if (!header_skipped) {
            header_skipped = true;
            if (line.find("ts") != std::string::npos && line.find("limitPx") != std::string::npos) {
                continue;
            }
        }

        try {
            const auto fields = split_csv_simple(line);
            if (fields.size() < 5) {
                throw std::runtime_error("expected at least 5 CSV fields");
            }

            ExternalOrderEvent ev{};
            ev.ts = parse_timestamp_stub(fields[0]);
            ev.price = std::stod(fields[1]);
            ev.size = std::stod(fields[2]);
            ev.isAsk = parse_bool(fields[3]);
            ev.statusId = std::stoi(fields[4]);

            if (fields.size() >= 6) {
                ev.statusText = fields[5];
                ev.eventType = map_event_type(fields[5]);
            } else {
                ev.statusText = "";
                ev.eventType = map_event_type(std::to_string(ev.statusId));
            }

            events.push_back(ev);
        } catch (const std::exception &ex) {
            if (log_errors) {
                std::cerr << "Malformed Hyperliquid CSV row at line " << line_number << ": "
                          << ex.what() << "\n";
            }
            if (strict_mode) {
                throw;
            }
        }
    }

    return events;
}

EventType HyperliquidCsvReader::map_event_type(const std::string &statusText) const {
    const std::string s = trim(statusText);

    if (s == "open" || s == "resting" || s == "received") {
        return EventType::New;
    }
    if (s == "canceled" || s == "cancelled") {
        return EventType::Cancel;
    }
    if (s == "filled" || s == "partiallyFilled" || s == "partialFill") {
        return EventType::Fill;
    }
    if (s == "triggered" || s == "trigger") {
        return EventType::Trigger;
    }
    if (s.find("Rejected") != std::string::npos || s.find("rejected") != std::string::npos) {
        return EventType::Reject;
    }

    if (s == "1") {
        return EventType::New;
    }
    if (s == "2") {
        return EventType::Cancel;
    }
    if (s == "3") {
        return EventType::Fill;
    }
    if (s == "4") {
        return EventType::Reject;
    }
    if (s == "5") {
        return EventType::Trigger;
    }

    return EventType::Other;
}

} // namespace bookforge