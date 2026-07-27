#include "snapshot/SnapshotBuilder.hpp"

#include "core/matching_engine.hpp"
#include "core/order.hpp"
#include "core/order_book.hpp"

namespace bookforge {

BookSnapshot SnapshotBuilder::Build(const MatchingEngine &engine, const SnapshotBuildContext &ctx,
                                    std::size_t depth_levels) {
    BookSnapshot snapshot{};
    snapshot.symbol = ctx.symbol;
    snapshot.replay_event_index = ctx.replay_event_index;
    snapshot.replay_timestamp_ns = ctx.replay_timestamp_ns;
    snapshot.total_events_seen = ctx.total_events_seen;
    snapshot.submitted_orders = ctx.submitted_orders;
    snapshot.rejected_events = ctx.rejected_events;
    snapshot.ignored_events = ctx.ignored_events;
    snapshot.generated_trades = ctx.generated_trades;

    const OrderBook &book = engine.Book();

    snapshot.best_bid = book.GetBestBid();
    snapshot.best_ask = book.GetBestAsk();
    snapshot.mid_price = book.GetMidPrice();
    snapshot.spread = book.GetSpread();

    const auto bid_depth = book.GetBidDepth(depth_levels);
    const auto ask_depth = book.GetAskDepth(depth_levels);

    snapshot.bids.reserve(bid_depth.size());
    snapshot.asks.reserve(ask_depth.size());

    for (const auto &[price, qty] : bid_depth) {
        snapshot.bids.push_back(DepthLevelSnapshot{price, qty});
    }

    for (const auto &[price, qty] : ask_depth) {
        snapshot.asks.push_back(DepthLevelSnapshot{price, qty});
    }

    return snapshot;
}

} // namespace bookforge