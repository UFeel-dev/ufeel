#ifndef HEARTRATE_HEARTRATESOURCE_HPP
#define HEARTRATE_HEARTRATESOURCE_HPP

#include "UFeelHeartRateStatus.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>

struct HeartRateResult
{
    std::uint32_t bpm;
    HeartRateStatus status;
};

class HeartRateSource
{
public:

    HeartRateSource() = default;

    virtual ~HeartRateSource() = default;

    HeartRateSource(const HeartRateSource&) = delete;
    auto operator=(const HeartRateSource&) -> HeartRateSource& = delete;
    HeartRateSource(HeartRateSource&&) = delete;
    auto operator=(HeartRateSource&&) -> HeartRateSource& = delete;

    [[nodiscard]]
    virtual auto read() const -> HeartRateResult = 0;
};

class LatestReading
{
public:

    void set(HeartRateResult result);

    [[nodiscard]]
    auto get() const -> HeartRateResult;

private:

    using Clock = std::chrono::steady_clock;

    static constexpr std::chrono::seconds kStaleAfter{5};

    mutable std::mutex mutex_;
    HeartRateResult result_{};
    Clock::time_point time_;
    bool hasData_{false};
};

#endif // HEARTRATE_HEARTRATESOURCE_HPP
