#include <benchmark/benchmark.h>

#include <cstdint>

#include "core/order_book.hpp"

using namespace bookforge;

namespace {

Order MakeOrder(std::uint64_t id,
                std::uint64_t participant_id,
                Side side,
                double price,
                std::uint32_t quantity,
                std::uint64_t timestamp,
                SelfTradePrevention stp = SelfTradePrevention::None) {
    return Order{id, participant_id, side, price, quantity, timestamp, stp};
}

static void BM_AddOrder(benchmark::State& state) {
    const auto iterations_per_run = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
            const double price =
                (side == Side::Buy)
                    ? 100.00 - static_cast<double>(i % 50) * 0.01
                    : 100.50 + static_cast<double>(i % 50) * 0.01;
            const std::uint32_t quantity = 100 + static_cast<std::uint32_t>(i % 25);
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);

            book.AddOrder(MakeOrder(id, id, side, price, quantity, id));
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_AddOrder)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_CancelOrder(benchmark::State& state) {
    const auto iterations_per_run = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
            const double price =
                (side == Side::Buy)
                    ? 100.00 - static_cast<double>(i % 50) * 0.01
                    : 100.50 + static_cast<double>(i % 50) * 0.01;
            const std::uint32_t quantity = 100;
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);

            book.AddOrder(MakeOrder(id, id, side, price, quantity, id));
        }

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            book.CancelOrder(static_cast<std::uint64_t>(i + 1));
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_CancelOrder)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_ExecuteTopOrderPartial(benchmark::State& state) {
    const auto iterations_per_run = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
            book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));
        }

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            book.ExecuteTopOrder(Side::Buy, 100.00, 1);
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ExecuteTopOrderPartial)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_ExecuteTopOrderFull(benchmark::State& state) {
    const auto iterations_per_run = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
            book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));
        }

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            book.ExecuteTopOrder(Side::Buy, 100.00, 100);
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ExecuteTopOrderFull)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_ReduceOrderQuantity(benchmark::State& state) {
    const auto iterations_per_run = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
            book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));
        }

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
            book.ReduceOrderQuantity(id, 99);
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ReduceOrderQuantity)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_ReplaceOrderSamePrice(benchmark::State& state) {
    const auto iterations_per_run = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
            book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));
        }

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
            const std::uint64_t new_timestamp =
                static_cast<std::uint64_t>(iterations_per_run + i + 1);
            book.ReplaceOrder(id, 100.00, 99, new_timestamp);
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ReplaceOrderSamePrice)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_ReplaceOrderNewPrice(benchmark::State& state) {
    const auto iterations_per_run = static_cast<std::size_t>(state.range(0));

    for (auto _ : state) {
        OrderBook book;

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
            const double price = 100.00 + static_cast<double>(i % 10) * 0.01;
            book.AddOrder(MakeOrder(id, id, Side::Buy, price, 100, id));
        }

        for (std::size_t i = 0; i < iterations_per_run; ++i) {
            const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
            const double new_price = 101.00 + static_cast<double>(i % 10) * 0.01;
            const std::uint64_t new_timestamp =
                static_cast<std::uint64_t>(iterations_per_run + i + 1);
            book.ReplaceOrder(id, new_price, 99, new_timestamp);
        }

        benchmark::DoNotOptimize(book);
    }

    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_ReplaceOrderNewPrice)->Arg(1000)->Arg(10000)->Arg(100000);

}  // namespace