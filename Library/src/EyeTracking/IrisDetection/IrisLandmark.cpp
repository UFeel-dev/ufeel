#include "EyeTracking/IrisDetection/IrisLandmark.hpp"

#include "EyeTracking/IrisDetection/FaceDetection.hpp"
#include "EyeTracking/IrisDetection/FaceLandmark.hpp"
#include "opencv2/core/types.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

enum
{
    EYE_LANDMARKS = 71,
    IRIS_LANDMARKS = 5
};

static auto isEyeIndexValid(
    int idx
) -> bool
{
    if (idx < 0 || idx >= EYE_LANDMARKS)
    {
        std::cerr << "Index " << idx << " is out of range (" << EYE_LANDMARKS << ")." << '\n';
        return false;
    }
    return true;
}

static auto isIrisIndexValid(
    int idx
) -> bool
{
    if (idx < 0 || idx >= IRIS_LANDMARKS)
    {
        std::cerr << "Index " << idx << " is out of range (" << IRIS_LANDMARKS << ")." << '\n';
        return false;
    }
    return true;
}

my::IrisLandmark::IrisLandmark(
    const std::string& modelPath
)
    : FaceLandmark(modelPath)
    , m_leftIrisLandmarker(modelPath + std::string("/iris_landmark.tflite"))
    , m_rightIrisLandmarker(modelPath + std::string("/iris_landmark.tflite"))
{
}

void my::IrisLandmark::runInference()
{
    FaceLandmark::runInference();
    auto roi = FaceDetection::getFaceRoi();
    if (roi.empty())
    {
        return;
    }

    std::thread t([this]() -> void { this->runEyeInference(true); });
    runEyeInference(false);
    t.join();
}

auto my::IrisLandmark::getEyeLandmarkAt(
    int index, bool isLeftEye, bool isIris
) const -> cv::Point
{
    if (isEyeIndexValid(index))
    {
        const auto* model = isLeftEye ? &m_leftIrisLandmarker : &m_rightIrisLandmarker;
        auto eyeRoi = isLeftEye ? m_leftEyeRoi : m_rightEyeRoi;

        float const _x =
            model->getOutputData(static_cast<int>(isIris))[static_cast<ptrdiff_t>(index * 3)];
        float const _y = model->getOutputData(static_cast<int>(isIris))[(index * 3) + 1];

        int const x = static_cast<int>(_x / model->getInputShape()[2] * eyeRoi.width) + eyeRoi.x;
        int const y = static_cast<int>(_y / model->getInputShape()[1] * eyeRoi.height) + eyeRoi.y;

        return {x, y};
    }
    return {};
}

auto my::IrisLandmark::getAllEyeLandmarks(
    bool isLeftEye, bool isIris
) const -> std::vector<cv::Point>
{
    if (my::FaceDetection::getFaceRoi().empty())
    {
        return {};
    }

    int const n = isIris ? IRIS_LANDMARKS : EYE_LANDMARKS;

    std::vector<cv::Point> landmarks(n);
    for (int i = 0; i < n; ++i)
    {
        landmarks[i] = getEyeLandmarkAt(i, isLeftEye, isIris);
    }
    return landmarks;
}

auto my::IrisLandmark::loadOutput(
    int /*index*/, bool isLeftEye
) const -> std::vector<float>
{
    const auto* model = isLeftEye ? &m_leftIrisLandmarker : &m_rightIrisLandmarker;
    return model->loadOutput();
}

auto my::IrisLandmark::getEyeRoi(
    bool isLeftEye
) const -> cv::Rect
{
    return isLeftEye ? m_leftEyeRoi : m_rightEyeRoi;
}

auto my::IrisLandmark::calculateEyeRoi(
    cv::Point leftMoft, cv::Point rightMost
) -> cv::Rect
{
    int const cx = (leftMoft.x + rightMost.x) / 2;
    int const cy = (leftMoft.y + rightMost.y) / 2;

    int w = std::abs(leftMoft.x - rightMost.x);
    int h = std::abs(leftMoft.y - rightMost.y);
    w = h = std::max(w, h);

    return {cx - (w / 2), cy - (h / 2), w, h};
}

void my::IrisLandmark::runEyeInference(
    bool isLeftEye
)
{
    int const idx1 = isLeftEye ? 446 : 244;
    int const idx2 = isLeftEye ? 464 : 226;

    auto* roi = isLeftEye ? &m_leftEyeRoi : &m_rightEyeRoi;
    auto* model = isLeftEye ? &m_leftIrisLandmarker : &m_rightIrisLandmarker;

    auto pt1 = FaceLandmark::getFaceLandmarkAt(idx1);
    auto pt2 = FaceLandmark::getFaceLandmarkAt(idx2);

    *roi = calculateEyeRoi(pt1, pt2);
    auto eyePatch = FaceDetection::cropFrame(*roi);

    model->loadImageToInput(eyePatch);
    model->runInference();
}
