#include "EyeTracking/IrisDetection/IrisLandmark.hpp"

#include "EyeTracking/IrisDetection/FaceDetection.hpp"
#include "EyeTracking/IrisDetection/FaceLandmark.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <opencv2/core/types.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{

constexpr int EYE_LANDMARKS = 71;
constexpr int IRIS_LANDMARKS = 5;

auto isLandmarkIndexValid(
    int index, int count
) -> bool
{
    return index >= 0 && index < count;
}

} // namespace

my::IrisLandmark::IrisLandmark(
    const std::string& modelPath
)
    : FaceLandmark(modelPath)
    , m_leftIrisLandmarker(modelPath + "/iris_landmark.tflite")
    , m_rightIrisLandmarker(modelPath + "/iris_landmark.tflite")
{
}

void my::IrisLandmark::runInference()
{
    m_leftEyeRoi = {};
    m_rightEyeRoi = {};

    FaceLandmark::runInference();

    if (FaceDetection::getFaceRoi().empty())
    {
        return;
    }

    std::exception_ptr leftException;
    std::exception_ptr rightException;

    std::thread leftThread([this, &leftException]() {
        try
        {
            runEyeInference(true);
        }
        catch (...)
        {
            leftException = std::current_exception();
        }
    });

    try
    {
        runEyeInference(false);
    }
    catch (...)
    {
        rightException = std::current_exception();
    }

    leftThread.join();

    if (leftException || rightException)
    {
        m_leftEyeRoi = {};
        m_rightEyeRoi = {};

        if (rightException)
        {
            std::rethrow_exception(rightException);
        }

        std::rethrow_exception(leftException);
    }
}

auto my::IrisLandmark::getEyeLandmarkAt(
    int index, bool isLeftEye, bool isIris
) const -> cv::Point
{
    int const landmarkCount = isIris ? IRIS_LANDMARKS : EYE_LANDMARKS;
    if (!isLandmarkIndexValid(index, landmarkCount))
    {
        return {};
    }

    auto const* model = isLeftEye ? &m_leftIrisLandmarker : &m_rightIrisLandmarker;
    auto const eyeRoi = isLeftEye ? m_leftEyeRoi : m_rightEyeRoi;

    if (eyeRoi.empty())
    {
        return {};
    }

    int const outputIndex = isIris ? 1 : 0;
    auto const outputSize = model->getOutputSize(outputIndex);
    auto const requiredSize = static_cast<std::size_t>(index + 1) * 3 * sizeof(float);

    if (outputSize < requiredSize)
    {
        return {};
    }

    auto const inputShape = model->getInputShape();
    if (inputShape.size() < 3 || inputShape[1] <= 0 || inputShape[2] <= 0)
    {
        return {};
    }

    auto const* output = model->getOutputData(outputIndex);
    if (output == nullptr)
    {
        return {};
    }

    float const x = output[static_cast<ptrdiff_t>(index * 3)];
    float const y = output[(index * 3) + 1];

    if (!std::isfinite(x) || !std::isfinite(y))
    {
        return {};
    }

    int const imageX =
        static_cast<int>(x / static_cast<float>(inputShape[2]) * eyeRoi.width) + eyeRoi.x;
    int const imageY =
        static_cast<int>(y / static_cast<float>(inputShape[1]) * eyeRoi.height) + eyeRoi.y;

    return {imageX, imageY};
}

auto my::IrisLandmark::getAllEyeLandmarks(
    bool isLeftEye, bool isIris
) const -> std::vector<cv::Point>
{
    auto const eyeRoi = isLeftEye ? m_leftEyeRoi : m_rightEyeRoi;
    if (eyeRoi.empty())
    {
        return {};
    }

    int const count = isIris ? IRIS_LANDMARKS : EYE_LANDMARKS;
    std::vector<cv::Point> landmarks;
    landmarks.reserve(count);

    for (int index = 0; index < count; ++index)
    {
        landmarks.push_back(getEyeLandmarkAt(index, isLeftEye, isIris));
    }

    return landmarks;
}

auto my::IrisLandmark::loadOutput(
    int index, bool isLeftEye
) const -> std::vector<float>
{
    auto const* model = isLeftEye ? &m_leftIrisLandmarker : &m_rightIrisLandmarker;
    return model->loadOutput(index);
}

auto my::IrisLandmark::getEyeRoi(
    bool isLeftEye
) const -> cv::Rect
{
    return isLeftEye ? m_leftEyeRoi : m_rightEyeRoi;
}

auto my::IrisLandmark::calculateEyeRoi(
    cv::Point leftMost, cv::Point rightMost
) -> cv::Rect
{
    int const centerX = (leftMost.x + rightMost.x) / 2;
    int const centerY = (leftMost.y + rightMost.y) / 2;

    int const width = std::abs(leftMost.x - rightMost.x);
    int const height = std::abs(leftMost.y - rightMost.y);
    int const size = std::max(width, height);

    if (size <= 0)
    {
        return {};
    }

    return {centerX - (size / 2), centerY - (size / 2), size, size};
}

void my::IrisLandmark::runEyeInference(
    bool isLeftEye
)
{
    int const firstLandmarkIndex = isLeftEye ? 446 : 244;
    int const secondLandmarkIndex = isLeftEye ? 464 : 226;

    auto* eyeRoi = isLeftEye ? &m_leftEyeRoi : &m_rightEyeRoi;
    auto* model = isLeftEye ? &m_leftIrisLandmarker : &m_rightIrisLandmarker;

    auto const firstPoint = FaceLandmark::getFaceLandmarkAt(firstLandmarkIndex);
    auto const secondPoint = FaceLandmark::getFaceLandmarkAt(secondLandmarkIndex);

    *eyeRoi = calculateEyeRoi(firstPoint, secondPoint);
    if (eyeRoi->empty())
    {
        throw std::runtime_error("Unable to calculate a valid eye region.");
    }

    auto const eyePatch = FaceDetection::cropFrame(*eyeRoi);
    if (eyePatch.empty())
    {
        *eyeRoi = {};
        throw std::runtime_error("Unable to crop the eye region from the input frame.");
    }

    model->loadImageToInput(eyePatch);
    model->runInference();
}
