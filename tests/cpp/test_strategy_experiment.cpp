#include "replay/StrategyExperiment.hpp"

#include <gtest/gtest.h>

namespace bookforge {
namespace {

TEST(StrategyExperimentTest,
     MakeInjectedOrderMapsConfigFieldsIntoReplayOrder) {
  StrategyExperimentConfig config;
  config.mode = StrategyMode::Passive;
  config.entry_offset = 42;
  config.is_buy = true;
  config.limit_price = 101.25;
  config.quantity = 7;
  config.timing = InjectedOrderTiming::AfterEvent;

  const auto order = MakeInjectedOrder(config, "order-1", "p1");

  EXPECT_EQ(order.trigger_event_index, 42U);
  EXPECT_EQ(order.timing, InjectedOrderTiming::AfterEvent);
  EXPECT_EQ(order.order_id, "order-1");
  EXPECT_EQ(order.participant_id, "p1");
  EXPECT_TRUE(order.is_buy);
  EXPECT_DOUBLE_EQ(order.price, 101.25);
  EXPECT_EQ(order.quantity, 7U);
}

TEST(StrategyExperimentTest,
     MakeSingleOrderScheduleStoresOrderAtTriggerIndex) {
  InjectedOrder order;
  order.trigger_event_index = 5;
  order.timing = InjectedOrderTiming::BeforeEvent;
  order.order_id = "order-2";
  order.participant_id = "p2";
  order.is_buy = false;
  order.price = 99.5;
  order.quantity = 3;

  const auto schedule = MakeSingleOrderSchedule(order);
  const auto* orders = schedule.Find(5);

  ASSERT_NE(orders, nullptr);
  ASSERT_EQ(orders->size(), 1U);
  EXPECT_EQ((*orders)[0].order_id, "order-2");
  EXPECT_EQ((*orders)[0].participant_id, "p2");
  EXPECT_FALSE((*orders)[0].is_buy);
  EXPECT_DOUBLE_EQ((*orders)[0].price, 99.5);
  EXPECT_EQ((*orders)[0].quantity, 3U);
}

}  // namespace
}  // namespace bookforge