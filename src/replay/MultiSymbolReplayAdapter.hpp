#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "HyperliquidMatchingEngineAdapter.hpp"
#include "IReplayAdapter.hpp"
#include "core/matching_engine.hpp"

namespace bookforge {

class MultiSymbolReplayAdapter final : public IReplayAdapter {
  public:
    explicit MultiSymbolReplayAdapter(std::string fallback_symbol = {})
        : fallback_symbol_(std::move(fallback_symbol)) {}

    void OnEvent(const ExternalOrderEvent &event) override {
        const std::string symbol = ResolveSymbol(event);

        if (symbol.empty()) {
            ++metrics_.ignored;
            return;
        }

        HyperliquidMatchingEngineAdapter &adapter = GetOrCreateAdapter(symbol);
        adapter.OnEvent(event);
        AccumulateEventMetrics(event);
    }

    void OnInjectedOrder(const InjectedOrder &order) override {
        const std::string symbol = ResolveSymbol(order);

        if (symbol.empty()) {
            ++metrics_.unsupported;
            return;
        }

        HyperliquidMatchingEngineAdapter &adapter = GetOrCreateAdapter(symbol);
        adapter.OnInjectedOrder(order);
        ++metrics_.submitted;
    }

    const AdapterMetrics &Metrics() const override {
        return metrics_;
    }

    [[nodiscard]] const MatchingEngine *FindEngine(const std::string &symbol) const {
        const auto it = engines_.find(symbol);
        if (it == engines_.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    [[nodiscard]] const HyperliquidMatchingEngineAdapter *
    FindAdapter(const std::string &symbol) const {
        const auto it = adapters_.find(symbol);
        if (it == adapters_.end()) {
            return nullptr;
        }

        return it->second.get();
    }

    [[nodiscard]] std::vector<std::string> Symbols() const {
        std::vector<std::string> symbols;
        symbols.reserve(engines_.size());

        for (const auto &[symbol, engine] : engines_) {
            (void)engine;
            symbols.push_back(symbol);
        }

        std::sort(symbols.begin(), symbols.end());
        return symbols;
    }

    [[nodiscard]] std::size_t SymbolCount() const {
        return engines_.size();
    }

  private:
    std::string ResolveSymbol(const ExternalOrderEvent &event) const {
        if (!event.symbol.empty()) {
            return event.symbol;
        }

        return fallback_symbol_;
    }

    std::string ResolveSymbol(const InjectedOrder &order) const {
        if (!order.symbol.empty()) {
            return order.symbol;
        }

        return fallback_symbol_;
    }

    HyperliquidMatchingEngineAdapter &GetOrCreateAdapter(const std::string &symbol) {
        const auto adapter_it = adapters_.find(symbol);
        if (adapter_it != adapters_.end()) {
            return *adapter_it->second;
        }

        auto engine = std::make_unique<MatchingEngine>();
        MatchingEngine &engine_ref = *engine;

        engines_.emplace(symbol, std::move(engine));

        auto adapter = std::make_unique<HyperliquidMatchingEngineAdapter>(engine_ref);
        HyperliquidMatchingEngineAdapter &adapter_ref = *adapter;

        adapters_.emplace(symbol, std::move(adapter));

        return adapter_ref;
    }

    void AccumulateEventMetrics(const ExternalOrderEvent &event) {
        switch (event.eventType) {
        case EventType::New:
            ++metrics_.newEvents;
            ++metrics_.submitted;
            break;
        case EventType::Cancel:
            ++metrics_.cancelEvents;
            ++metrics_.unsupported;
            break;
        case EventType::Fill:
            ++metrics_.fillEvents;
            ++metrics_.unsupported;
            break;
        case EventType::Reject:
            ++metrics_.rejectEvents;
            ++metrics_.rejected;
            break;
        case EventType::Trigger:
            ++metrics_.triggerEvents;
            ++metrics_.ignored;
            break;
        case EventType::Other:
            ++metrics_.otherEvents;
            ++metrics_.ignored;
            break;
        }
    }

  private:
    std::string fallback_symbol_;
    std::unordered_map<std::string, std::unique_ptr<MatchingEngine>> engines_;
    std::unordered_map<std::string, std::unique_ptr<HyperliquidMatchingEngineAdapter>> adapters_;
    AdapterMetrics metrics_{};
};

} // namespace bookforge
