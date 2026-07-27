#pragma once

#include "ExternalOrderEvent.hpp"

#include <string>
#include <vector>

namespace bookforge {

class HyperliquidCsvReader {
  public:
    explicit HyperliquidCsvReader(std::string path);

    std::vector<ExternalOrderEvent> read_all();
    std::vector<ExternalOrderEvent> read_all(bool strict_mode, bool log_errors);

  private:
    std::string path_;

    EventType map_event_type(const std::string &statusText) const;
};

} // namespace bookforge