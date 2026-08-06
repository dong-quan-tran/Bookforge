#include "replay/StrategyExperimentCsvWriter.hpp"

#include <fstream>

namespace bookforge {

// static
bool StrategyExperimentCsvWriter::Write(const std::string &path,
                                        const std::vector<StrategyExperimentResult> &results) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }

    // Header row expected by tests:
    // "strategy,entry_offset,is_buy,limit_price,requested_qty,filled_qty,remaining_qty,
    //  fill_rate,avg_execution_price,decision_mid_price,decision_spread,
    //  implementation_shortfall_bps,time_to_first_fill_us,time_to_full_fill_us"
    out << "strategy,"
        << "entry_offset,"
        << "is_buy,"
        << "limit_price,"
        << "requested_qty,"
        << "filled_qty,"
        << "remaining_qty,"
        << "fill_rate,"
        << "avg_execution_price,"
        << "decision_mid_price,"
        << "decision_spread,"
        << "implementation_shortfall_bps,"
        << "time_to_first_fill_us,"
        << "time_to_full_fill_us"
        << "\n";

    for (const auto &r : results) {
        const char *strategy_str = (r.mode == StrategyMode::Aggressive) ? "aggressive" : "passive";

        const char *is_buy_str = r.is_buy ? "true" : "false";

        out << '"' << strategy_str << '"' << ',' << r.entry_offset << ',' << is_buy_str << ','
            << r.limit_price << ',' << r.requested_qty << ',' << r.filled_qty << ','
            << r.remaining_qty << ',' << r.fill_rate << ',' << r.avg_execution_price << ','
            << r.decision_mid_price << ',' << r.decision_spread << ','
            << r.implementation_shortfall_bps << ',' << r.time_to_first_fill_us << ','
            << r.time_to_full_fill_us << "\n";
    }

    return true;
}

} // namespace bookforge