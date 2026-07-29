#include <gtest/gtest.h>

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
    void OnEvent(const ExternalOrderEvent &) override { seen.push_back("event"); }

    void OnInjectedOrder(const InjectedOrder &order) override {
        seen.push_back("inject:" + order.order_id);
    }

    const AdapterMetrics &Metrics() const override { return metrics_; }

    AdapterMetrics metrics_{};
    std::vector<std::string> seen;
};

ExternalOrderEvent MakeEvent() {
    ExternalOrderEvent ev{};
    ev.eventType = EventType::New;
    ev.price = 100.0;
    ev.size = 1.0;
    ev.isAsk = false;
    return ev;
}

TEST(ReplayRunnerInjectedOrdersTest, DispatchesInjectedOrdersBeforeAndAfterConfiguredEvent) {
    ReplayConfig config{};
    ReplayRunner runner(config);

    std::vector<ExternalOrderEvent> events{
        MakeEvent(),
        MakeEvent(),
        MakeEvent(),
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
        "event",
        "inject:X",
        "event",
        "inject:Y",
        "event",
    };
    EXPECT_EQ(adapter.seen, expected);
}

} // namespace
} // namespace bookforge