#ifndef LIBRARY_TESTSUPPORT_FIXTURES_HPP
#define LIBRARY_TESTSUPPORT_FIXTURES_HPP

#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#ifndef RESOURCES_DIR_PATH
#define RESOURCES_DIR_PATH ""
#endif // RESOURCES_DIR_PATH

inline auto resourcePath(
    const std::string& name
) -> std::filesystem::path
{
    return std::filesystem::path(RESOURCES_DIR_PATH) / name;
}

inline auto loadFaceFrame() -> cv::Mat
{
    return cv::imread(resourcePath("astronaut.png").string(), cv::IMREAD_COLOR);
}

inline auto makeBlankFrame() -> cv::Mat
{
    return {480, 640, CV_8UC3, cv::Scalar::all(0)};
}

inline auto makeGreyscaleFaceFrame() -> cv::Mat
{
    cv::Mat grayscale;
    cv::cvtColor(loadFaceFrame(), grayscale, cv::COLOR_BGR2GRAY);
    return grayscale;
}

inline auto makeSmallFaceFrame() -> cv::Mat
{
    constexpr int faceSize = 24;
    cv::Mat face;
    cv::resize(loadFaceFrame(), face, cv::Size(faceSize, faceSize), 0.0, 0.0, cv::INTER_AREA);

    cv::Mat frame = makeBlankFrame();
    face.copyTo(frame(cv::Rect(0, 0, faceSize, faceSize)));
    return frame;
}

inline auto frameSequence(
    std::vector<cv::Mat> frames
) -> std::function<bool(cv::Mat&)>
{
    return [frames = std::move(frames), index = std::size_t{0}](cv::Mat& frame) mutable -> bool {
        if (index >= frames.size())
        {
            return false;
        }
        frame = frames.at(index++).clone();
        return true;
    };
}

#endif // LIBRARY_TESTSUPPORT_FIXTURES_HPP
