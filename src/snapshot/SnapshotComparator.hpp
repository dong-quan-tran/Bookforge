#pragma once

#include <string>

#include "snapshot/BookSnapshot.hpp"

namespace bookforge {

struct SnapshotComparisonResult {
    bool equal{true};
    std::string message;
};

class SnapshotComparator {
  public:
    [[nodiscard]] static SnapshotComparisonResult Compare(const BookSnapshot &lhs,
                                                          const BookSnapshot &rhs);
};

} // namespace bookforge