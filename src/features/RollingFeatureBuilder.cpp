#include "features/RollingFeatureBuilder.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace bookforge {
namespace {

double Mean(const std::vector<double> &values) {
    double sum = 0.0;
    for (double v : values) {
        sum += v;
    }
    return values.empty() ? 0.0 : (sum / static_cast<double>(values.size()));
}

} // namespace

void RollingFeatureBuilder::AddRollingContextFeatures(std::vector<FeatureRow> &rows,
                                                      std::size_t window) {
    if (window == 0) {
        throw std::runtime_error("rolling feature window must be > 0");
    }

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const std::size_t start = (i + 1 >= window) ? (i + 1 - window) : 0;

        std::vector<double> spreads;
        std::vector<double> l1_depths;
        std::vector<double> lN_depths;
        std::vector<double> abs_ofi_l1s;
        std::vector<double> abs_ofi_lNs;
        std::vector<double> one_step_mid_returns;

        for (std::size_t j = start; j <= i; ++j) {
            const auto &row = rows[j];

            if (row.spread.has_value()) {
                spreads.push_back(*row.spread);
            }

            if (row.l1_bid_qty.has_value() && row.l1_ask_qty.has_value()) {
                l1_depths.push_back(*row.l1_bid_qty + *row.l1_ask_qty);
            }

            if (row.lN_bid_qty_sum.has_value() && row.lN_ask_qty_sum.has_value()) {
                lN_depths.push_back(*row.lN_bid_qty_sum + *row.lN_ask_qty_sum);
            }

            if (row.ofi_l1.has_value()) {
                abs_ofi_l1s.push_back(std::fabs(*row.ofi_l1));
            }

            if (row.ofi_lN.has_value()) {
                abs_ofi_lNs.push_back(std::fabs(*row.ofi_lN));
            }

            if (j > start && rows[j - 1].mid_price.has_value() && row.mid_price.has_value() &&
                *rows[j - 1].mid_price > 0.0) {
                const double r = (*row.mid_price / *rows[j - 1].mid_price) - 1.0;
                one_step_mid_returns.push_back(r);
            }
        }

        if (!spreads.empty()) {
            rows[i].rolling_mean_spread = Mean(spreads);
        }

        if (!l1_depths.empty()) {
            rows[i].rolling_mean_l1_total_depth = Mean(l1_depths);
        }

        if (!lN_depths.empty()) {
            rows[i].rolling_mean_lN_total_depth = Mean(lN_depths);
        }

        if (!abs_ofi_l1s.empty()) {
            rows[i].rolling_mean_abs_ofi_l1 = Mean(abs_ofi_l1s);
        }

        if (!abs_ofi_lNs.empty()) {
            rows[i].rolling_mean_abs_ofi_lN = Mean(abs_ofi_lNs);
        }

        if (i >= start && rows[start].mid_price.has_value() && rows[i].mid_price.has_value() &&
            *rows[start].mid_price > 0.0 && i > start) {
            rows[i].rolling_mid_return = (*rows[i].mid_price / *rows[start].mid_price) - 1.0;
        }

        if (!one_step_mid_returns.empty()) {
            double sum_sq = 0.0;
            for (double r : one_step_mid_returns) {
                sum_sq += r * r;
            }
            rows[i].rolling_realized_mid_vol = std::sqrt(sum_sq);
        }
    }
}

} // namespace bookforge