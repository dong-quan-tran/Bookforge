#pragma once

#include <iosfwd>

#include "replay/ReplayRunner.hpp"

namespace bookforge {

void WriteReplayPacingSummary(std::ostream &output, const ReplayRunMetrics &metrics);

} // namespace bookforge