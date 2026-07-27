#pragma once

#include <string>
#include <vector>

#include "snapshot/BookSnapshot.hpp"

namespace bookforge {

class SnapshotSerializer {
  public:
    static void WriteCsv(const std::string &path, const std::vector<BookSnapshot> &snapshots,
                         std::size_t depth_levels);

  private:
    static std::string Header(std::size_t depth_levels);
    static std::string Row(const BookSnapshot &snapshot, std::size_t depth_levels);
};

} // namespace bookforge