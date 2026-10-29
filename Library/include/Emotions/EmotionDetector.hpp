#ifndef EMOTIONS_EMOTIONDETECTOR_HPP
#define EMOTIONS_EMOTIONDETECTOR_HPP

#include "Inference/HeadCalibration.hpp"
#include "opencv2/core/mat.hpp"
#include "Utils/FrameChecker.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <string>

class InferencePipeline;

class EmotionDetector
{
public:

    explicit EmotionDetector(const std::filesystem::path& modelDirectory);
    ~EmotionDetector();

    EmotionDetector(const EmotionDetector&) = delete;
    auto operator=(const EmotionDetector&) -> EmotionDetector& = delete;

    EmotionDetector(EmotionDetector&&) = delete;
    auto operator=(EmotionDetector&&) -> EmotionDetector& = delete;

    [[nodiscard]] auto process(const cv::Mat& image) -> std::map<std::string, float>;

private:

    std::unique_ptr<InferencePipeline> pipeline_;
    HeadCalibration calibration_;

    FrameChecker<std::map<std::string, float>> frameChecker_;
};

#endif // EMOTIONS_EMOTIONDETECTOR_HPP
