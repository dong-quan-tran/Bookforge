#pragma once

#include "ExternalOrderEvent.hpp"

namespace bookforge {

struct AdapterMetrics {
    std::size_t submitted{0};
    std::size_t ignored{0};
    std::size_t rejected{0};
    std::size_t unsupported{0};

    std::size_t newEvents{0};
    std::size_t cancelEvents{0};
    std::size_t fillEvents{0};
    std::size_t rejectEvents{0};
    std::size_t triggerEvents{0};
    std::size_t otherEvents{0};
};

class IReplayAdapter {
  public:
    virtual ~IReplayAdapter() = default;

    virtual void OnEvent(const ExternalOrderEvent &ev) = 0;
    virtual const AdapterMetrics &Metrics() const = 0;
};

} // namespace bookforge