#include "replay/ReplayRunner.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>

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

std::chrono::nanoseconds ComputePacingDelay(const ExternalOrderEvent &previous,
                                            const ExternalOrderEvent &current,
                                            double replay_speed) {
    if (replay_speed <= 0.0 || current.ts <= previous.ts) {
        return std::chrono::nanoseconds::zero();
    }

    const auto event_delta = current.ts - previous.ts;
    const auto scaled_count =
        static_cast<std::int64_t>(static_cast<double>(event_delta.count()) / replay_speed);

    if (scaled_count <= 0) {
        return std::chrono::nanoseconds::zero();
    }

    return std::chrono::nanoseconds{scaled_count};
}

} // namespace

ReplayRunner::ReplayRunner(const ReplayConfig &config)
    : config_(config), owned_clock_(std::make_unique<WallClockReplayClock>()),
      clock_(owned_clock_.get()) {}

ReplayRunner::ReplayRunner(const ReplayConfig &config, IReplayClock &clock)
    : config_(config), clock_(&clock) {}

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
    const ExternalOrderEvent *previous_event = nullptr;

    for (std::size_t i = start; i < total; ++i) {
        if (config_.max_events != 0 && processed >= static_cast<std::size_t>(config_.max_events)) {
            break;
        }

        if (config_.pacing_mode == ReplayPacingMode::EventTime && previous_event != nullptr &&
            clock_ != nullptr) {
            clock_->SleepFor(ComputePacingDelay(*previous_event, events[i], config_.replay_speed));
        }

        DispatchInjectedOrders(adapter, schedule, i, InjectedOrderTiming::BeforeEvent);
        adapter.OnEvent(events[i]);
        DispatchInjectedOrders(adapter, schedule, i, InjectedOrderTiming::AfterEvent);

        previous_event = &events[i];
        ++processed;

        if (config_.log_every_n != 0 &&
            processed % static_cast<std::size_t>(config_.log_every_n) == 0) {
            std::cout << "[ReplayRunner] processed=" << processed << '\n';
        }
    }

    if (config_.log_summary) {
        const auto &metrics = adapter.Metrics();
        std::cout << "[ReplayRunner] summary"
                  << " processed=" << processed << " submitted=" << metrics.submitted
                  << " ignored=" << metrics.ignored << " rejected=" << metrics.rejected
                  << " unsupported=" << metrics.unsupported << '\n';
    }

    return true;
}

} // namespace bookforge