#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "snapshot/BookSnapshot.hpp"

namespace bookforge {

class SnapshotBinaryDeserializer {
  public:
    [[nodiscard]] static std::vector<BookSnapshot> Read(const std::string &path,
                                                        std::size_t expected_depth_levels);
};

} // namespace bookforge