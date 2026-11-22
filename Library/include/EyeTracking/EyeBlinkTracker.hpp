#ifndef EYETRACKING_EYEBLINKTRACKER_HPP
#define EYETRACKING_EYEBLINKTRACKER_HPP

#include <chrono>
#include <cstdint>
#include <optional>

struct EyeBlinkState
{
    float leftOpenness = 0.0F;
    float rightOpenness = 0.0F;
    bool leftClosed = false;
    bool rightClosed = false;
    std::uint32_t blinkCount = 0;
    float eyesClosedSeconds = 0.0F;
};

class EyeBlinkTracker
{
public:

    using Clock = std::chrono::steady_clock;

    static constexpr double CLOSE_THRESHOLD = 0.55;
    static constexpr double REOPEN_THRESHOLD = 0.7;
    static constexpr auto MAXIMUM_BLINK_DURATION = std::chrono::milliseconds(1000);

    auto update(double leftAspectRatio, double rightAspectRatio, Clock::time_point now)
        -> EyeBlinkState;

    void interrupt();

    [[nodiscard]] auto state() const -> const EyeBlinkState&;

private:

    struct Eye
    {
        double openLevel = 0.0;
        bool closed = false;

        auto update(double aspectRatio) -> double;
    };

    Eye left_;
    Eye right_;
    std::optional<Clock::time_point> closedSince_;
    EyeBlinkState state_;
};

#endif // EYETRACKING_EYEBLINKTRACKER_HPP
