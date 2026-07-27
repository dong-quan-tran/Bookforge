#include "features/FeatureCsvWriter.hpp"

#include <fstream>
#include <optional>
#include <stdexcept>

namespace bookforge {
namespace {

void WriteOptional(std::ofstream &out, const std::optional<double> &value) {
    if (value.has_value()) {
        out << *value;
    }
}

} // namespace

void FeatureCsvWriter::Write(const std::string &path, const std::vector<FeatureRow> &rows) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("failed to open feature CSV for writing: " + path);
    }

    out << "symbol"
        << ",replay_event_index"
        << ",replay_timestamp_ns"
        << ",best_bid"
        << ",best_ask"
        << ",spread"
        << ",mid_price"
        << ",l1_bid_qty"
        << ",l1_ask_qty"
        << ",l1_depth_imbalance"
        << ",lN_bid_qty_sum"
        << ",lN_ask_qty_sum"
        << ",lN_depth_imbalance"
        << ",ofi_l1"
        << ",ofi_lN"
        << ",weighted_ofi_lN"
        << ",rolling_mean_spread"
        << ",rolling_mean_l1_total_depth"
        << ",rolling_mean_lN_total_depth"
        << ",rolling_mid_return"
        << ",rolling_realized_mid_vol"
        << ",rolling_mean_abs_ofi_l1"
        << ",rolling_mean_abs_ofi_lN"
        << "\n";

    for (const auto &row : rows) {
        out << row.symbol << "," << row.replay_event_index << "," << row.replay_timestamp_ns << ",";

        WriteOptional(out, row.best_bid);
        out << ",";
        WriteOptional(out, row.best_ask);
        out << ",";
        WriteOptional(out, row.spread);
        out << ",";
        WriteOptional(out, row.mid_price);
        out << ",";
        WriteOptional(out, row.l1_bid_qty);
        out << ",";
        WriteOptional(out, row.l1_ask_qty);
        out << ",";
        WriteOptional(out, row.l1_depth_imbalance);
        out << ",";
        WriteOptional(out, row.lN_bid_qty_sum);
        out << ",";
        WriteOptional(out, row.lN_ask_qty_sum);
        out << ",";
        WriteOptional(out, row.lN_depth_imbalance);
        out << ",";
        WriteOptional(out, row.ofi_l1);
        out << ",";
        WriteOptional(out, row.ofi_lN);
        out << ",";
        WriteOptional(out, row.weighted_ofi_lN);
        out << ",";
        WriteOptional(out, row.rolling_mean_spread);
        out << ",";
        WriteOptional(out, row.rolling_mean_l1_total_depth);
        out << ",";
        WriteOptional(out, row.rolling_mean_lN_total_depth);
        out << ",";
        WriteOptional(out, row.rolling_mid_return);
        out << ",";
        WriteOptional(out, row.rolling_realized_mid_vol);
        out << ",";
        WriteOptional(out, row.rolling_mean_abs_ofi_l1);
        out << ",";
        WriteOptional(out, row.rolling_mean_abs_ofi_lN);
        out << "\n";
    }
}

} // namespace bookforge