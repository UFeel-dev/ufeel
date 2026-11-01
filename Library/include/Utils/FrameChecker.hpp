#ifndef UTILS_FRAMECHECKER_HPP_
#define UTILS_FRAMECHECKER_HPP_

#include "opencv2/core.hpp"
#include "opencv2/core/mat.hpp"

template <typename T> class FrameChecker
{
public:

    FrameChecker() = default;
    FrameChecker(const FrameChecker&) = delete;
    FrameChecker(FrameChecker&&) = delete;
    auto operator=(const FrameChecker&) -> FrameChecker& = delete;
    auto operator=(FrameChecker&&) -> FrameChecker& = delete;
    ~FrameChecker() = default;

    [[nodiscard]] auto isFrameIdentical(
        const cv::Mat& frame
    ) const -> bool
    {
        if (!hasPreviousFrame_ || frame.empty())
        {
            return false;
        }

        if (frame.size() != previousFrame_.size() || frame.type() != previousFrame_.type())
        {
            return false;
        }

        cv::Mat diff;
        cv::bitwise_xor(frame, previousFrame_, diff);

        return cv::countNonZero(diff.reshape(1)) == 0;
    }

    auto setFrame(
        const cv::Mat& frame
    ) -> void
    {
        if (frame.empty())
        {
            previousFrame_.release();
            hasPreviousFrame_ = false;
            return;
        }

        frame.copyTo(previousFrame_);
        hasPreviousFrame_ = true;
    }

    auto setResult(
        const T& result
    ) -> void
    {
        previousResult_ = result;
    }

    auto getResult() const -> const T&
    {
        return previousResult_;
    }

    auto hasPreviousFrame() const -> bool
    {
        return hasPreviousFrame_;
    }

private:

    cv::Mat previousFrame_;
    T previousResult_{};
    bool hasPreviousFrame_ = false;
};

#endif // UTILS_FRAMECHECKER_HPP_
