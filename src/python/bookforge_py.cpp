#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "features/FeatureRow.hpp"
#include "snapshot/BookSnapshot.hpp"

namespace py = pybind11;

PYBIND11_MODULE(bookforge_py, m) {
    m.doc() = "Python bindings for Bookforge snapshot and feature objects";

    py::class_<bookforge::DepthLevelSnapshot>(m, "DepthLevelSnapshot")
        .def(py::init<>())
        .def_readwrite("price", &bookforge::DepthLevelSnapshot::price)
        .def_readwrite("quantity", &bookforge::DepthLevelSnapshot::quantity);

    py::class_<bookforge::BookSnapshot>(m, "BookSnapshot")
        .def(py::init<>())
        .def_readwrite("symbol", &bookforge::BookSnapshot::symbol)
        .def_readwrite("replay_event_index", &bookforge::BookSnapshot::replay_event_index)
        .def_readwrite("replay_timestamp_ns", &bookforge::BookSnapshot::replay_timestamp_ns)
        .def_readwrite("total_events_seen", &bookforge::BookSnapshot::total_events_seen)
        .def_readwrite("submitted_orders", &bookforge::BookSnapshot::submitted_orders)
        .def_readwrite("rejected_events", &bookforge::BookSnapshot::rejected_events)
        .def_readwrite("ignored_events", &bookforge::BookSnapshot::ignored_events)
        .def_readwrite("generated_trades", &bookforge::BookSnapshot::generated_trades)
        .def_readwrite("best_bid", &bookforge::BookSnapshot::best_bid)
        .def_readwrite("best_ask", &bookforge::BookSnapshot::best_ask)
        .def_readwrite("mid_price", &bookforge::BookSnapshot::mid_price)
        .def_readwrite("spread", &bookforge::BookSnapshot::spread)
        .def_readwrite("bids", &bookforge::BookSnapshot::bids)
        .def_readwrite("asks", &bookforge::BookSnapshot::asks);

    py::class_<bookforge::FeatureRow>(m, "FeatureRow")
        .def(py::init<>())
        .def_readwrite("symbol", &bookforge::FeatureRow::symbol)
        .def_readwrite("replay_event_index", &bookforge::FeatureRow::replay_event_index)
        .def_readwrite("replay_timestamp_ns", &bookforge::FeatureRow::replay_timestamp_ns)
        .def_readwrite("best_bid", &bookforge::FeatureRow::best_bid)
        .def_readwrite("best_ask", &bookforge::FeatureRow::best_ask)
        .def_readwrite("spread", &bookforge::FeatureRow::spread)
        .def_readwrite("mid_price", &bookforge::FeatureRow::mid_price)
        .def_readwrite("l1_bid_qty", &bookforge::FeatureRow::l1_bid_qty)
        .def_readwrite("l1_ask_qty", &bookforge::FeatureRow::l1_ask_qty)
        .def_readwrite("l1_depth_imbalance", &bookforge::FeatureRow::l1_depth_imbalance)
        .def_readwrite("lN_bid_qty_sum", &bookforge::FeatureRow::lN_bid_qty_sum)
        .def_readwrite("lN_ask_qty_sum", &bookforge::FeatureRow::lN_ask_qty_sum)
        .def_readwrite("lN_depth_imbalance", &bookforge::FeatureRow::lN_depth_imbalance)
        .def_readwrite("ofi_l1", &bookforge::FeatureRow::ofi_l1)
        .def_readwrite("ofi_lN", &bookforge::FeatureRow::ofi_lN)
        .def_readwrite("weighted_ofi_lN", &bookforge::FeatureRow::weighted_ofi_lN)
        .def_readwrite("rolling_mean_spread", &bookforge::FeatureRow::rolling_mean_spread)
        .def_readwrite("rolling_mean_l1_total_depth",
                       &bookforge::FeatureRow::rolling_mean_l1_total_depth)
        .def_readwrite("rolling_mean_lN_total_depth",
                       &bookforge::FeatureRow::rolling_mean_lN_total_depth)
        .def_readwrite("rolling_mid_return", &bookforge::FeatureRow::rolling_mid_return)
        .def_readwrite("rolling_realized_mid_vol", &bookforge::FeatureRow::rolling_realized_mid_vol)
        .def_readwrite("rolling_mean_abs_ofi_l1", &bookforge::FeatureRow::rolling_mean_abs_ofi_l1)
        .def_readwrite("rolling_mean_abs_ofi_lN", &bookforge::FeatureRow::rolling_mean_abs_ofi_lN);
}