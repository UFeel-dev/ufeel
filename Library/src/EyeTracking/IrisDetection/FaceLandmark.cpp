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
    if (isFaceLandmarkIndexValid(index))
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
