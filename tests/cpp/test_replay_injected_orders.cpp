#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

#include "ExternalOrderEvent.hpp"
#include "IReplayAdapter.hpp"
#include "replay/InjectedOrderSchedule.hpp"
#include "replay/ReplayConfig.hpp"
#include "replay/ReplayRunner.hpp"

namespace bookforge {
namespace {

class RecordingReplayAdapter final : public IReplayAdapter {
  public:
    void OnEvent(const ExternalOrderEvent &) override {
        seen.push_back("event");
    }

    void OnInjectedOrder(const InjectedOrder &order) override {
        seen.push_back("inject:" + order.order_id);
        injected_timestamps.push_back(order.replay_timestamp_ns);
    }

    const AdapterMetrics &Metrics() const override {
        return metrics_;
    }

    AdapterMetrics metrics_{};
    std::vector<std::string> seen;
    std::vector<std::uint64_t> injected_timestamps;
};

ExternalOrderEvent MakeEvent(std::int64_t timestamp_ns) {
    ExternalOrderEvent event{};
    event.ts = std::chrono::nanoseconds{timestamp_ns};
    event.eventType = EventType::New;
    event.price = 100.0;
    event.size = 1.0;
    event.isAsk = false;
    return event;
}

TEST(ReplayRunnerInjectedOrdersTest, DispatchesInjectedOrdersBeforeAndAfterConfiguredEvent) {
    ReplayConfig config{};
    ReplayRunner runner(config);

    std::vector<ExternalOrderEvent> events{
        MakeEvent(100),
        MakeEvent(200),
        MakeEvent(300),
    };

    InjectedOrderSchedule schedule;
    schedule.Add(InjectedOrder{
        .trigger_event_index = 1,
        .timing = InjectedOrderTiming::BeforeEvent,
        .order_id = "X",
        .participant_id = "demo",
        .is_buy = true,
        .price = 99.5,
        .quantity = 1000,
    });
    schedule.Add(InjectedOrder{
        .trigger_event_index = 1,
        .timing = InjectedOrderTiming::AfterEvent,
        .order_id = "Y",
        .participant_id = "demo",
        .is_buy = false,
        .price = 100.5,
        .quantity = 1200,
    });

    RecordingReplayAdapter adapter;
    ASSERT_TRUE(runner.Run(adapter, events, schedule));

    const std::vector<std::string> expected{
        "event", "inject:X", "event", "inject:Y", "event",
    };

    EXPECT_EQ(adapter.seen, expected);

    const std::vector<std::uint64_t> expected_timestamps{
        200,
        200,
    };

    EXPECT_EQ(adapter.injected_timestamps, expected_timestamps);
}

} // namespace
} // namespace bookforge
