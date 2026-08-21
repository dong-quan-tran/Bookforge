#include "replay/ReplayMetricsReporter.hpp"

#include <ostream>

namespace bookforge {

void WriteReplayPacingSummary(std::ostream &output, const ReplayRunMetrics &metrics) {
    const auto &histogram = metrics.requested_pacing_delays;

    if (histogram.Empty()) {
        return;
    }

    output << "Requested pacing delays:\n"
           << "  samples: " << histogram.Count() << '\n'
           << "  total_ns: " << histogram.Total().count() << '\n'
           << "  min_ns: " << histogram.Min().count() << '\n'
           << "  max_ns: " << histogram.Max().count() << '\n'
           << "  overflow: " << histogram.OverflowCount() << '\n';

    for (const auto &bucket : histogram.Buckets()) {
        output << "  <= " << bucket.upper_bound.count() << " ns: " << bucket.count << '\n';
    }
}

} // namespace bookforge