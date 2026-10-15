#include "EyeTracking/IrisDetection/FaceLandmark.hpp"

#include "EyeTracking/IrisDetection/FaceDetection.hpp"
#include "opencv2/core/types.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

enum
{
    FACE_LANDMARKS = 468
};

/*
Helper function
*/
static auto isFaceLandmarkIndexValid(
    int idx
) -> bool
{
    if (idx < 0 || idx >= FACE_LANDMARKS)
    {
        std::cerr << "Index " << idx << " is out of range (" << FACE_LANDMARKS << ")." << '\n';
        return false;
    }
    return true;
}

my::FaceLandmark::FaceLandmark(
    const std::string& modelPath
)
    : FaceDetection(modelPath)
    , m_landmarkModel(modelPath + std::string("/face_landmark.tflite"))
{
}

void my::FaceLandmark::runInference()
{
    FaceDetection::runInference();
    auto roi = FaceDetection::getFaceRoi();
    if (roi.empty())
    {
        return;
    }

    auto face = FaceDetection::cropFrame(roi);
    m_landmarkModel.loadImageToInput(face);
    m_landmarkModel.runInference();
}

auto my::FaceLandmark::getFaceLandmarkAt(
    int index
) const -> cv::Point
{
    if (isFaceLandmarkIndexValid(index) && !getFaceRoi().empty() &&
        m_landmarkModel.getOutputSize() >=
            static_cast<std::size_t>((index + 1) * 3) * sizeof(float))
    {
        auto roi = FaceDetection::getFaceRoi();

        float const _x = m_landmarkModel.getOutputData()[static_cast<ptrdiff_t>(index * 3)];
        float const _y = m_landmarkModel.getOutputData()[(index * 3) + 1];

        int const x = static_cast<int>(_x / m_landmarkModel.getInputShape()[2] * roi.width) + roi.x;
        int const y =
            static_cast<int>(_y / m_landmarkModel.getInputShape()[1] * roi.height) + roi.y;

        return {x, y};
    }
    return {};
}

auto my::FaceLandmark::getFaceLandmark3DAt(
    int index
) const -> cv::Point3f
{
    if (!isFaceLandmarkIndexValid(index) || getFaceRoi().empty() ||
        m_landmarkModel.getOutputSize() < static_cast<std::size_t>((index + 1) * 3) * sizeof(float))
    {
        return {};
    }

    auto const roi = getFaceRoi();
    auto const inputShape = m_landmarkModel.getInputShape();
    if (inputShape.size() < 3 || inputShape[1] <= 0 || inputShape[2] <= 0)
    {
        return {};
    }

    auto const* output = m_landmarkModel.getOutputData();
    if (output == nullptr)
    {
        return {};
    }
    float const x = output[index * 3];
    float const y = output[(index * 3) + 1];
    float const z = output[(index * 3) + 2];
    float const depthScale = static_cast<float>(roi.width) / static_cast<float>(inputShape[2]);
    return {
        static_cast<float>(roi.x) + (x / static_cast<float>(inputShape[2]) * roi.width),
        static_cast<float>(roi.y) + (y / static_cast<float>(inputShape[1]) * roi.height),
        z * depthScale};
}

auto my::FaceLandmark::getAllFaceLandmarks() const -> std::vector<cv::Point>
{
    if (FaceDetection::getFaceRoi().empty())
    {
        return {};
    }

    std::vector<cv::Point> landmarks(FACE_LANDMARKS);
    for (int i = 0; i < FACE_LANDMARKS; ++i)
    {
        landmarks[i] = getFaceLandmarkAt(i);
    }
    return landmarks;
}

auto my::FaceLandmark::loadOutput(
    int /*index*/
) const -> std::vector<float>
{
    return m_landmarkModel.loadOutput();
}
