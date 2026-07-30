#include "replay/StrategyExperiment.hpp"

#include <catch2/catch_test_macros.hpp>

namespace bookforge {
namespace {

TEST_CASE("MakeInjectedOrder maps config fields into replay order") {
  StrategyExperimentConfig config;
  config.mode = StrategyMode::Passive;
  config.entry_offset = 42;
  config.is_buy = true;
  config.limit_price = 101.25;
  config.quantity = 7;
  config.timing = InjectedOrderTiming::AfterEvent;

  const auto order = MakeInjectedOrder(config, "order-1", "p1");

  REQUIRE(order.trigger_event_index == 42);
  REQUIRE(order.timing == InjectedOrderTiming::AfterEvent);
  REQUIRE(order.order_id == "order-1");
  REQUIRE(order.participant_id == "p1");
  REQUIRE(order.is_buy);
  REQUIRE(order.price == 101.25);
  REQUIRE(order.quantity == 7);
}

TEST_CASE("MakeSingleOrderSchedule stores order at trigger index") {
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

  REQUIRE(orders != nullptr);
  REQUIRE(orders->size() == 1);
  REQUIRE((*orders)[0].order_id == "order-2");
  REQUIRE((*orders)[0].participant_id == "p2");
  REQUIRE_FALSE((*orders)[0].is_buy);
  REQUIRE((*orders)[0].price == 99.5);
  REQUIRE((*orders)[0].quantity == 3);
}

}  // namespace
}  // namespace bookforge