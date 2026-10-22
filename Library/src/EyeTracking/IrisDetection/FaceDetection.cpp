#include "EyeTracking/IrisDetection/FaceDetection.hpp"

#include "EyeTracking/IrisDetection/DetectionPostProcess.hpp"
#include "EyeTracking/IrisDetection/ModelLoader.hpp"

#include <cmath>
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

my::FaceDetection::FaceDetection(
    const std::string& modelDir
)
    : my::ModelLoader(modelDir + "/face_detection_short.tflite")
{
}

void my::FaceDetection::loadImageToInput(
    const cv::Mat& inputImage, int /*idx*/
)
{
    m_roi = {};
    m_originImage = inputImage;

    ModelLoader::loadImageToInput(inputImage, 0);
}

void my::FaceDetection::runInference()
{
    m_roi = {};

    ModelLoader::runInference();

    auto const regressor = getFaceRegressor();
    auto const classificator = getFaceClassificator();
    auto const detection = m_postProcessor.getHighestScoreDetection(regressor, classificator);

    if (detection.classId != -1)
    {
        m_roi = calculateRoiFromDetection(detection);
    }
}

auto my::FaceDetection::getOriginalImage() const -> cv::Mat
{
    return m_originImage;
}

auto my::FaceDetection::getFaceRegressor() const -> std::vector<float>
{
    return ModelLoader::loadOutput(0);
}

auto my::FaceDetection::getFaceClassificator() const -> std::vector<float>
{
    return ModelLoader::loadOutput(1);
}

auto my::FaceDetection::getFaceRoi() const -> cv::Rect
{
    return m_roi;
}

auto my::FaceDetection::cropFrame(
    const cv::Rect& roi
) const -> cv::Mat
{
    auto const& frame = m_originImage;

    if (frame.empty() || roi.width <= 0 || roi.height <= 0)
    {
        return {};
    }

    cv::Mat cropped(roi.size(), frame.type(), cv::Scalar::all(0));
    cv::Rect const frameBounds(0, 0, frame.cols, frame.rows);
    cv::Rect const intersection = roi & frameBounds;

    if (intersection.empty())
    {
        return cropped;
    }

    cv::Point const destinationOffset = intersection.tl() - roi.tl();
    cv::Rect const destinationRegion(destinationOffset, intersection.size());

    frame(intersection).copyTo(cropped(destinationRegion));

    return cropped;
}

auto my::FaceDetection::calculateRoiFromDetection(
    const Detection& detection
) const -> cv::Rect
{
    if (m_originImage.empty())
    {
        return {};
    }

    int const originalWidth = m_originImage.cols;
    int const originalHeight = m_originImage.rows;

    auto const center = (detection.roi.tl() + detection.roi.br()) * 0.5f;

    float const centerX = center.x * static_cast<float>(originalWidth);
    float const centerY = center.y * static_cast<float>(originalHeight);
    float const width = detection.roi.width * static_cast<float>(originalWidth) * 1.5f;
    float const height = detection.roi.height * static_cast<float>(originalHeight) * 2.0f;

    if (!std::isfinite(centerX) || !std::isfinite(centerY) || !std::isfinite(width) ||
        !std::isfinite(height) || width < 1.0f || height < 1.0f)
    {
        return {};
    }

    return {
        static_cast<int>(centerX - (width / 2.0f)),
        static_cast<int>(centerY - (height / 2.0f)),
        static_cast<int>(width),
        static_cast<int>(height)};
}
