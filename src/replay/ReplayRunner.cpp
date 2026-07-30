#include "replay/ReplayRunner.hpp"

#include <cstddef>
#include <iostream>

namespace bookforge {
namespace {

void DispatchInjectedOrders(IReplayAdapter &adapter, const InjectedOrderSchedule &schedule,
                            std::size_t event_index, InjectedOrderTiming timing) {
    const auto *orders = schedule.Find(event_index);
    if (orders == nullptr) {
        return;
    }

    for (const auto &order : *orders) {
        if (order.timing == timing) {
            adapter.OnInjectedOrder(order);
        }
    }
}

} // namespace

bool ReplayRunner::Run(IReplayAdapter &adapter,
                       const std::vector<ExternalOrderEvent> &events) const {
    InjectedOrderSchedule empty_schedule;
    return Run(adapter, events, empty_schedule);
}

bool ReplayRunner::Run(IReplayAdapter &adapter, const std::vector<ExternalOrderEvent> &events,
                       const InjectedOrderSchedule &schedule) const {
    const std::size_t total = events.size();
    const std::size_t start =
        static_cast<std::size_t>(config_.start_offset > total ? total : config_.start_offset);

    std::size_t processed = 0;

    for (std::size_t i = start; i < total; ++i) {
        if (config_.max_events != 0 && processed >= static_cast<std::size_t>(config_.max_events)) {
            break;
        }

        DispatchInjectedOrders(adapter, schedule, i, InjectedOrderTiming::BeforeEvent);
        adapter.OnEvent(events[i]);
        DispatchInjectedOrders(adapter, schedule, i, InjectedOrderTiming::AfterEvent);

        ++processed;

        if (config_.log_every_n != 0 &&
            processed % static_cast<std::size_t>(config_.log_every_n) == 0) {
            std::cout << "[ReplayRunner] processed=" << processed << '\n';
        }
    }

    if (config_.log_summary) {
        const auto &m = adapter.Metrics();
        std::cout << "[ReplayRunner] summary"
                  << " processed=" << processed << " submitted=" << m.submitted
                  << " ignored=" << m.ignored << " rejected=" << m.rejected
                  << " unsupported=" << m.unsupported << '\n';
    }

    return true;
}

} // namespace bookforge