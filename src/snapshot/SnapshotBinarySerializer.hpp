#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "snapshot/BookSnapshot.hpp"

namespace bookforge {

class SnapshotBinarySerializer {
  public:
    static void Write(const std::string &path, const std::vector<BookSnapshot> &snapshots,
                      std::size_t depth_levels);
};

} // namespace bookforge