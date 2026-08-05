#include "replay/StrategyExperiment.hpp"

namespace bookforge {

InjectedOrder MakeInjectedOrder(const StrategyExperimentConfig &config, const std::string &order_id,
                                const std::string &participant_id) {
    InjectedOrder order;
    order.trigger_event_index = static_cast<std::size_t>(config.entry_offset);
    order.timing = config.timing;
    order.order_id = order_id;
    order.participant_id = participant_id;
    order.is_buy = config.is_buy;
    order.price = config.limit_price;
    order.quantity = config.quantity;
    return order;
}

InjectedOrderSchedule MakeSingleOrderSchedule(const InjectedOrder &order) {
    InjectedOrderSchedule schedule;
    schedule.Add(order);
    return schedule;
}

} // namespace bookforge