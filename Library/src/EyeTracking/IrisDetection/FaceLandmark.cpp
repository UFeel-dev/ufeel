#include "EyeTracking/IrisDetection/FaceLandmark.hpp"

#include "EyeTracking/IrisDetection/FaceDetection.hpp"

#include <cmath>
#include <cstddef>
#include <opencv2/core/types.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

constexpr int FACE_LANDMARKS = 468;

auto isFaceLandmarkIndexValid(
    int index
) -> bool
{
    return index >= 0 && index < FACE_LANDMARKS;
}

auto hasValidInputShape(
    const std::vector<int>& shape
) -> bool
{
    return shape.size() >= 3 && shape[1] > 0 && shape[2] > 0;
}

} // namespace

my::FaceLandmark::FaceLandmark(
    const std::string& modelPath
)
    : FaceDetection(modelPath)
    , m_landmarkModel(modelPath + "/face_landmark.tflite")
{
}

void my::FaceLandmark::runInference()
{
    FaceDetection::runInference();

    auto const roi = FaceDetection::getFaceRoi();
    if (roi.empty())
    {
        return;
    }

    auto const face = FaceDetection::cropFrame(roi);
    if (face.empty())
    {
        throw std::runtime_error("Unable to crop the detected face.");
    }

    m_landmarkModel.loadImageToInput(face);
    m_landmarkModel.runInference();
}

auto my::FaceLandmark::getFaceLandmarkAt(
    int index
) const -> cv::Point
{
    if (!isFaceLandmarkIndexValid(index) || getFaceRoi().empty())
    {
        return {};
    }

    auto const requiredSize = static_cast<std::size_t>(index + 1) * 3 * sizeof(float);
    if (m_landmarkModel.getOutputSize() < requiredSize)
    {
        return {};
    }

    auto const roi = FaceDetection::getFaceRoi();
    auto const inputShape = m_landmarkModel.getInputShape();
    auto const* output = m_landmarkModel.getOutputData();

    if (!hasValidInputShape(inputShape) || output == nullptr)
    {
        return {};
    }

    float const x = output[static_cast<ptrdiff_t>(index * 3)];
    float const y = output[(index * 3) + 1];

    if (!std::isfinite(x) || !std::isfinite(y))
    {
        return {};
    }

    int const imageX = static_cast<int>(x / static_cast<float>(inputShape[2]) * roi.width) + roi.x;
    int const imageY = static_cast<int>(y / static_cast<float>(inputShape[1]) * roi.height) + roi.y;

    return {imageX, imageY};
}

auto my::FaceLandmark::getFaceLandmark3DAt(
    int index
) const -> cv::Point3f
{
    if (!isFaceLandmarkIndexValid(index) || getFaceRoi().empty())
    {
        return {};
    }

    auto const requiredSize = static_cast<std::size_t>(index + 1) * 3 * sizeof(float);
    if (m_landmarkModel.getOutputSize() < requiredSize)
    {
        return {};
    }

    auto const roi = FaceDetection::getFaceRoi();
    auto const inputShape = m_landmarkModel.getInputShape();
    auto const* output = m_landmarkModel.getOutputData();

    if (!hasValidInputShape(inputShape) || output == nullptr)
    {
        return {};
    }

    float const x = output[static_cast<ptrdiff_t>(index * 3)];
    float const y = output[(index * 3) + 1];
    float const z = output[(index * 3) + 2];

    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
    {
        return {};
    }

    auto const inputWidth = static_cast<float>(inputShape[2]);
    auto const inputHeight = static_cast<float>(inputShape[1]);
    float const depthScale = static_cast<float>(roi.width) / inputWidth;

    return {
        static_cast<float>(roi.x) + (x / inputWidth * static_cast<float>(roi.width)),
        static_cast<float>(roi.y) + (y / inputHeight * static_cast<float>(roi.height)),
        z * depthScale};
}

auto my::FaceLandmark::getAllFaceLandmarks() const -> std::vector<cv::Point>
{
    if (FaceDetection::getFaceRoi().empty())
    {
        return {};
    }

    std::vector<cv::Point> landmarks;
    landmarks.reserve(FACE_LANDMARKS);

    for (int index = 0; index < FACE_LANDMARKS; ++index)
    {
        landmarks.push_back(getFaceLandmarkAt(index));
    }

    return landmarks;
}

auto my::FaceLandmark::loadOutput(
    int /*index*/
) const -> std::vector<float>
{
    return m_landmarkModel.loadOutput();
}
