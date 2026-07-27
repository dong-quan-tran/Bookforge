#pragma once

#include <string>
#include <vector>

#include "snapshot/BookSnapshot.hpp"

namespace bookforge {

class SnapshotDeserializer {
  public:
    [[nodiscard]] static std::vector<BookSnapshot> ReadCsv(const std::string &path,
                                                           std::size_t depth_levels);

  private:
    [[nodiscard]] static std::vector<std::string> SplitCsvLine(const std::string &line);
    [[nodiscard]] static std::vector<std::string> ExpectedHeader(std::size_t depth_levels);
    static void ValidateHeader(const std::vector<std::string> &actual, std::size_t depth_levels);
    [[nodiscard]] static BookSnapshot ParseRow(const std::vector<std::string> &cells,
                                               std::size_t depth_levels);
};

} // namespace bookforge