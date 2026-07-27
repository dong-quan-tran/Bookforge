#include <chrono>
#include <cstdint>
#include <iostream>

#include "core/order_book.hpp"

using namespace bookforge;

namespace {
using Clock = std::chrono::steady_clock;

Order MakeOrder(std::uint64_t id, std::uint64_t participant_id, Side side, double price,
                std::uint32_t quantity, std::uint64_t timestamp,
                SelfTradePrevention stp = SelfTradePrevention::None) {
    return Order{id, participant_id, side, price, quantity, timestamp, stp};
}

void PrintResult(const char *name, std::size_t iterations,
                 std::chrono::steady_clock::time_point start,
                 std::chrono::steady_clock::time_point end) {
    const auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << name << ": total_ns=" << total_ns
              << ", avg_ns=" << (total_ns / static_cast<long long>(iterations)) << '\n';
}

void BenchmarkAddOrder(std::size_t iterations) {
    OrderBook book;

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        const Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
        const double price = (side == Side::Buy) ? 100.00 - static_cast<double>(i % 50) * 0.01
                                                 : 100.50 + static_cast<double>(i % 50) * 0.01;
        const std::uint32_t quantity = 100 + static_cast<std::uint32_t>(i % 25);
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);

        book.AddOrder(MakeOrder(id, id, side, price, quantity, id));
    }
    const auto end = Clock::now();

    PrintResult("AddOrder", iterations, start, end);
}

void BenchmarkCancelOrder(std::size_t iterations) {
    OrderBook book;

    for (std::size_t i = 0; i < iterations; ++i) {
        const Side side = (i % 2 == 0) ? Side::Buy : Side::Sell;
        const double price = (side == Side::Buy) ? 100.00 - static_cast<double>(i % 50) * 0.01
                                                 : 100.50 + static_cast<double>(i % 50) * 0.01;
        const std::uint32_t quantity = 100;
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);

        book.AddOrder(MakeOrder(id, id, side, price, quantity, id));
    }

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        book.CancelOrder(static_cast<std::uint64_t>(i + 1));
    }
    const auto end = Clock::now();

    PrintResult("CancelOrder", iterations, start, end);
}

void BenchmarkExecuteTopOrderPartial(std::size_t iterations) {
    OrderBook book;

    for (std::size_t i = 0; i < iterations; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
        book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));
    }

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        book.ExecuteTopOrder(Side::Buy, 100.00, 1);
    }
    const auto end = Clock::now();

    PrintResult("ExecuteTopOrderPartial", iterations, start, end);
}

void BenchmarkExecuteTopOrderFull(std::size_t iterations) {
    OrderBook book;

    for (std::size_t i = 0; i < iterations; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
        book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));
    }

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        book.ExecuteTopOrder(Side::Buy, 100.00, 100);
    }
    const auto end = Clock::now();

    PrintResult("ExecuteTopOrderFull", iterations, start, end);
}

void BenchmarkReduceOrderQuantity(std::size_t iterations) {
    OrderBook book;

    for (std::size_t i = 0; i < iterations; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
        book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));
    }

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
        book.ReduceOrderQuantity(id, 99);
    }
    const auto end = Clock::now();

    PrintResult("ReduceOrderQuantity", iterations, start, end);
}

void BenchmarkReplaceOrderSamePrice(std::size_t iterations) {
    OrderBook book;

    for (std::size_t i = 0; i < iterations; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
        book.AddOrder(MakeOrder(id, id, Side::Buy, 100.00, 100, id));
    }

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
        const std::uint64_t new_timestamp = static_cast<std::uint64_t>(iterations + i + 1);
        book.ReplaceOrder(id, 100.00, 99, new_timestamp);
    }
    const auto end = Clock::now();

    PrintResult("ReplaceOrderSamePrice", iterations, start, end);
}

void BenchmarkReplaceOrderNewPrice(std::size_t iterations) {
    OrderBook book;

    for (std::size_t i = 0; i < iterations; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
        const double price = 100.00 + static_cast<double>(i % 10) * 0.01;
        book.AddOrder(MakeOrder(id, id, Side::Buy, price, 100, id));
    }

    const auto start = Clock::now();
    for (std::size_t i = 0; i < iterations; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(i + 1);
        const double new_price = 101.00 + static_cast<double>(i % 10) * 0.01;
        const std::uint64_t new_timestamp = static_cast<std::uint64_t>(iterations + i + 1);
        book.ReplaceOrder(id, new_price, 99, new_timestamp);
    }
    const auto end = Clock::now();

    PrintResult("ReplaceOrderNewPrice", iterations, start, end);
}

} // namespace

int main() {
    constexpr std::size_t kIterations = 100000;

    std::cout << "Bookforge OrderBook benchmark\n";
    std::cout << "iterations=" << kIterations << '\n';

    BenchmarkAddOrder(kIterations);
    BenchmarkCancelOrder(kIterations);
    BenchmarkExecuteTopOrderPartial(kIterations);
    BenchmarkExecuteTopOrderFull(kIterations);
    BenchmarkReduceOrderQuantity(kIterations);
    BenchmarkReplaceOrderSamePrice(kIterations);
    BenchmarkReplaceOrderNewPrice(kIterations);

    return 0;
}