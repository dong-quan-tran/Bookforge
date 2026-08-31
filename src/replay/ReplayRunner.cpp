#include "replay/ReplayRunner.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace bookforge {
namespace {

using namespace std::chrono_literals;

std::vector<std::chrono::nanoseconds> DefaultPacingDelayBounds() {
    return {
        1us, 10us, 100us, 1ms, 10ms, 100ms, 1s,
    };
}

void DispatchInjectedOrders(IReplayAdapter &adapter, const InjectedOrderSchedule &schedule,
                            std::size_t event_index, InjectedOrderTiming timing,
                            std::chrono::nanoseconds replay_timestamp) {
    const auto *orders = schedule.Find(event_index);
    if (orders == nullptr) {
        return;
    }

    for (const InjectedOrder &scheduled_order : *orders) {
        if (scheduled_order.timing != timing) {
            continue;
        }

        InjectedOrder order = scheduled_order;
        order.replay_timestamp_ns =
            replay_timestamp.count() < 0 ? 0 : static_cast<std::uint64_t>(replay_timestamp.count());

        adapter.OnInjectedOrder(order);
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
      clock_(owned_clock_.get()), metrics_{LatencyHistogram(DefaultPacingDelayBounds())} {}

ReplayRunner::ReplayRunner(const ReplayConfig &config, IReplayClock &clock)
    : config_(config), clock_(&clock), metrics_{LatencyHistogram(DefaultPacingDelayBounds())} {}

bool ReplayRunner::Run(IReplayAdapter &adapter,
                       const std::vector<ExternalOrderEvent> &events) const {
    InjectedOrderSchedule empty_schedule;
    return Run(adapter, events, empty_schedule);
}

bool ReplayRunner::Run(IReplayAdapter &adapter, const std::vector<ExternalOrderEvent> &events,
                       const InjectedOrderSchedule &schedule) const {
    metrics_ = ReplayRunMetrics{
        .requested_pacing_delays = LatencyHistogram(DefaultPacingDelayBounds()),
    };

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
            const auto delay = ComputePacingDelay(*previous_event, events[i], config_.replay_speed);

            if (delay > std::chrono::nanoseconds::zero()) {
                metrics_.requested_pacing_delays.Record(delay);
                clock_->SleepFor(delay);
            }
        }

        DispatchInjectedOrders(adapter, schedule, i, InjectedOrderTiming::BeforeEvent,
                               events[i].ts);
        adapter.OnEvent(events[i]);
        DispatchInjectedOrders(adapter, schedule, i, InjectedOrderTiming::AfterEvent, events[i].ts);

        previous_event = &events[i];
        ++processed;

        if (config_.log_every_n != 0 &&
            processed % static_cast<std::size_t>(config_.log_every_n) == 0) {
            std::cout << "[ReplayRunner] processed=" << processed << '\n';
        }
    }

    if (config_.log_summary) {
        const AdapterMetrics &adapter_metrics = adapter.Metrics();
        std::cout << "[ReplayRunner] summary"
                  << " processed=" << processed << " submitted=" << adapter_metrics.submitted
                  << " ignored=" << adapter_metrics.ignored
                  << " rejected=" << adapter_metrics.rejected
                  << " unsupported=" << adapter_metrics.unsupported << '\n';
    }

    return true;
}

const ReplayRunMetrics &ReplayRunner::Metrics() const {
    return metrics_;
}

} // namespace bookforge
