#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>

#include "core/order_book.hpp"

using namespace bookforge;

namespace {

Order MakeOrder(std::uint64_t id, std::uint64_t participant_id, Side side, double price,
                std::uint32_t quantity, std::uint64_t timestamp,
                SelfTradePrevention stp = SelfTradePrevention::None) {
    return Order{id, participant_id, side, price, quantity, timestamp, stp};
}

double BidPriceForIndex(std::size_t index) {
    return 100.00 - static_cast<double>(index % 1000) * 0.01;
}

double AskPriceForIndex(std::size_t index) {
    return 100.50 + static_cast<double>(index % 1000) * 0.01;
}

void PopulateOneSidedBook(OrderBook &book, Side side, std::size_t order_count,
                          std::uint32_t quantity = 100) {
    for (std::size_t index = 0; index < order_count; ++index) {
        const std::uint64_t id = static_cast<std::uint64_t>(index + 1);
        const double price = side == Side::Buy ? BidPriceForIndex(index) : AskPriceForIndex(index);

        const bool added = book.AddOrder(MakeOrder(id, id, side, price, quantity, id));
        benchmark::DoNotOptimize(added);
    }
}

static void BM_AddOrderIsolated(benchmark::State &state) {
    for (auto _ : state) {
        state.PauseTiming();

        OrderBook book;
        const std::uint64_t id = static_cast<std::uint64_t>(state.iterations()) + 1;

        state.ResumeTiming();

        const bool added = book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));

        benchmark::DoNotOptimize(added);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_AddOrderIsolated);

static void BM_CancelOrderIsolated(benchmark::State &state) {
    const std::size_t order_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook book;
        PopulateOneSidedBook(book, Side::Buy, order_count);
        const std::uint64_t target_id = static_cast<std::uint64_t>(order_count);

        state.ResumeTiming();

        const bool canceled = book.CancelOrder(target_id);

        benchmark::DoNotOptimize(canceled);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_CancelOrderIsolated)->Arg(1'000)->Arg(10'000)->Arg(100'000);

static void BM_ExecuteTopOrderPartialIsolated(benchmark::State &state) {
    const std::size_t order_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook book;
        PopulateOneSidedBook(book, Side::Sell, order_count);
        const double best_ask_price = AskPriceForIndex(0);

        state.ResumeTiming();

        const bool executed = book.ExecuteTopOrder(Side::Sell, best_ask_price, 1);

        benchmark::DoNotOptimize(executed);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ExecuteTopOrderPartialIsolated)->Arg(1'000)->Arg(10'000)->Arg(100'000);

static void BM_ExecuteTopOrderFullIsolated(benchmark::State &state) {
    const std::size_t order_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook book;
        PopulateOneSidedBook(book, Side::Sell, order_count);
        const double best_ask_price = AskPriceForIndex(0);

        state.ResumeTiming();

        const bool executed = book.ExecuteTopOrder(Side::Sell, best_ask_price, 100);

        benchmark::DoNotOptimize(executed);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ExecuteTopOrderFullIsolated)->Arg(1'000)->Arg(10'000)->Arg(100'000);

static void BM_ReduceOrderQuantityIsolated(benchmark::State &state) {
    const std::size_t order_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook book;
        PopulateOneSidedBook(book, Side::Buy, order_count);
        const std::uint64_t target_id = static_cast<std::uint64_t>(order_count);

        state.ResumeTiming();

        const bool reduced = book.ReduceOrderQuantity(target_id, 99);

        benchmark::DoNotOptimize(reduced);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_ReduceOrderQuantityIsolated)->Arg(1'000)->Arg(10'000)->Arg(100'000);

static void BM_RequeueOrderAtNewPriceIsolated(benchmark::State &state) {
    const std::size_t order_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();

        OrderBook book;
        PopulateOneSidedBook(book, Side::Buy, order_count);
        const std::uint64_t target_id = static_cast<std::uint64_t>(order_count);

        state.ResumeTiming();

        const bool replaced =
            book.ReplaceOrder(target_id, 101.00, 99, static_cast<std::uint64_t>(order_count + 1));

        benchmark::DoNotOptimize(replaced);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_RequeueOrderAtNewPriceIsolated)->Arg(1'000)->Arg(10'000)->Arg(100'000);

static void BM_AddOrderWorkload(benchmark::State &state) {
    const std::size_t order_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t index = 0; index < order_count; ++index) {
            const std::uint64_t id = static_cast<std::uint64_t>(index + 1);
            const Side side = index % 2 == 0 ? Side::Buy : Side::Sell;
            const double price =
                side == Side::Buy ? BidPriceForIndex(index) : AskPriceForIndex(index);

            const bool added = book.AddOrder(MakeOrder(id, id, side, price, 100, id));
            benchmark::DoNotOptimize(added);
        }

        benchmark::DoNotOptimize(book);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}

BENCHMARK(BM_AddOrderWorkload)->Arg(1'000)->Arg(10'000)->Arg(100'000);

static void BM_AddThenCancelWorkload(benchmark::State &state) {
    const std::size_t order_count = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t index = 0; index < order_count; ++index) {
            const std::uint64_t id = static_cast<std::uint64_t>(index + 1);
            const Side side = index % 2 == 0 ? Side::Buy : Side::Sell;
            const double price =
                side == Side::Buy ? BidPriceForIndex(index) : AskPriceForIndex(index);

            const bool added = book.AddOrder(MakeOrder(id, id, side, price, 100, id));
            benchmark::DoNotOptimize(added);
        }

        for (std::size_t index = 0; index < order_count; ++index) {
            const std::uint64_t id = static_cast<std::uint64_t>(index + 1);
            const bool canceled = book.CancelOrder(id);
            benchmark::DoNotOptimize(canceled);
        }

        benchmark::DoNotOptimize(book);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * state.range(0) * 2);
}

BENCHMARK(BM_AddThenCancelWorkload)->Arg(1'000)->Arg(10'000)->Arg(100'000);

} // namespace
