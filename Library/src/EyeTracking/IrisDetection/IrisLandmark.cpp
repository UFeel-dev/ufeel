#include "EyeTracking/IrisDetection/IrisLandmark.hpp"

#include "EyeTracking/IrisDetection/FaceDetection.hpp"
#include "EyeTracking/IrisDetection/FaceLandmark.hpp"
#include "EyeTracking/IrisDetection/ModelLoader.hpp"
#include "opencv2/core.hpp"
#include "opencv2/core/base.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgproc.hpp"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{

constexpr int EYE_LANDMARKS = 71;
constexpr int IRIS_LANDMARKS = 5;
constexpr float EYE_CROP_SCALE = 2.3F;

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
    , leftIrisLandmarker_(std::make_unique<ModelLoader>(modelPath + "/iris_landmark.tflite"))
    , rightIrisLandmarker_(std::make_unique<ModelLoader>(modelPath + "/iris_landmark.tflite"))
{
    leftIrisLandmarker_->setImageInputRange(0.0F, 1.0F);
    rightIrisLandmarker_->setImageInputRange(0.0F, 1.0F);
}

my::IrisLandmark::IrisLandmark(
    const std::string& modelPath,
    std::unique_ptr<my::ModelLoader> faceLandmarkModel,
    std::unique_ptr<my::ModelLoader> leftIrisLandmarkModel,
    std::unique_ptr<my::ModelLoader> rightIrisLandmarkModel
)
    : FaceLandmark(modelPath, std::move(faceLandmarkModel))
    , leftIrisLandmarker_(std::move(leftIrisLandmarkModel))
    , rightIrisLandmarker_(std::move(rightIrisLandmarkModel))
{
    leftIrisLandmarker_->setImageInputRange(0.0F, 1.0F);
    rightIrisLandmarker_->setImageInputRange(0.0F, 1.0F);
}

void my::IrisLandmark::runInference()
{
    leftEyeRoi_ = {};
    rightEyeRoi_ = {};
    leftCropToImage_.release();
    rightCropToImage_.release();

    FaceLandmark::runInference();

    if (FaceDetection::getFaceRoi().empty())
    {
        return;
    }

    std::exception_ptr leftException;
    std::exception_ptr rightException;

    std::thread leftThread([this, &leftException] -> void {
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
        leftEyeRoi_ = {};
        rightEyeRoi_ = {};

        if (rightException)
        {
            std::rethrow_exception(rightException);
        }

        std::rethrow_exception(leftException);
    }
}

auto my::IrisLandmark::getEyeLandmarkAt(
    int index, bool isLeftEye, bool isIris
) const -> cv::Point2f
{
    int const landmarkCount = isIris ? IRIS_LANDMARKS : EYE_LANDMARKS;
    if (!isLandmarkIndexValid(index, landmarkCount))
    {
        return {};
    }

    auto const* model = isLeftEye ? leftIrisLandmarker_.get() : rightIrisLandmarker_.get();
    auto const& cropToImage = isLeftEye ? leftCropToImage_ : rightCropToImage_;

    if (cropToImage.empty())
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

    std::vector<cv::Point2f> mapped;
    cv::transform(std::vector<cv::Point2f>{{x, y}}, mapped, cropToImage);
    return mapped.front();
}

auto my::IrisLandmark::getAllEyeLandmarks(
    bool isLeftEye, bool isIris
) const -> std::vector<cv::Point2f>
{
    auto const eyeRoi = isLeftEye ? leftEyeRoi_ : rightEyeRoi_;
    if (eyeRoi.empty())
    {
        return {};
    }

    int const count = isIris ? IRIS_LANDMARKS : EYE_LANDMARKS;
    std::vector<cv::Point2f> landmarks;
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
    auto const* model = isLeftEye ? leftIrisLandmarker_.get() : rightIrisLandmarker_.get();
    return model->loadOutput(index);
}

auto my::IrisLandmark::getEyeRoi(
    bool isLeftEye
) const -> cv::Rect
{
    return isLeftEye ? leftEyeRoi_ : rightEyeRoi_;
}

auto my::IrisLandmark::calculateEyeTransform(
    cv::Point2f corner1, cv::Point2f corner2, cv::Size cropSize, bool mirror
) -> cv::Mat
{
    float const distance = static_cast<float>(cv::norm(corner2 - corner1));
    if (!std::isfinite(distance) || distance < 1.0F || cropSize.width <= 0 || cropSize.height <= 0)
    {
        return {};
    }

    cv::Point2f const center = (corner1 + corner2) * 0.5F;
    double const angle =
        std::atan2(corner2.y - corner1.y, corner2.x - corner1.x) * 180.0 / std::numbers::pi;
    double const scale = static_cast<double>(cropSize.width) / (distance * EYE_CROP_SCALE);
    cv::Mat imageToCrop = cv::getRotationMatrix2D(center, angle, scale);
    imageToCrop.at<double>(0, 2) += (cropSize.width / 2.0) - center.x;
    imageToCrop.at<double>(1, 2) += (cropSize.height / 2.0) - center.y;
    if (mirror)
    {
        imageToCrop.row(0) *= -1.0;
        imageToCrop.at<double>(0, 2) += cropSize.width;
    }
    return imageToCrop;
}

void my::IrisLandmark::runEyeInference(
    bool isLeftEye
)
{
    int const firstCorner = isLeftEye ? 362 : 33;
    int const secondCorner = isLeftEye ? 263 : 133;

    auto* eyeRoi = isLeftEye ? &leftEyeRoi_ : &rightEyeRoi_;
    auto* cropToImage = isLeftEye ? &leftCropToImage_ : &rightCropToImage_;
    auto* model = isLeftEye ? leftIrisLandmarker_.get() : rightIrisLandmarker_.get();

    auto const inputShape = model->getInputShape();
    if (inputShape.size() < 3 || inputShape.at(1) <= 0 || inputShape.at(2) <= 0)
    {
        throw std::runtime_error("Unsupported iris model input shape.");
    }
    cv::Size const cropSize(inputShape.at(2), inputShape.at(1));

    auto const imageToCrop = calculateEyeTransform(
        getFaceLandmarkAt(firstCorner), getFaceLandmarkAt(secondCorner), cropSize, isLeftEye
    );
    auto const image = FaceDetection::getOriginalImage();
    if (imageToCrop.empty() || image.empty())
    {
        throw std::runtime_error("Unable to calculate a valid eye region.");
    }

    cv::Mat eyePatch;
    cv::warpAffine(image, eyePatch, imageToCrop, cropSize, cv::INTER_LINEAR, cv::BORDER_REPLICATE);
    cv::invertAffineTransform(imageToCrop, *cropToImage);

    std::vector<cv::Point2f> center;
    cv::transform(
        std::vector<cv::Point2f>{
            {static_cast<float>(cropSize.width) / 2.0F,
             static_cast<float>(cropSize.height) / 2.0F}},
        center,
        *cropToImage
    );
    auto const side = static_cast<int>(std::lround(cropSize.width / cv::norm(imageToCrop.col(0))));
    *eyeRoi = {
        static_cast<int>(std::lround(center.front().x)) - (side / 2),
        static_cast<int>(std::lround(center.front().y)) - (side / 2),
        side,
        side};

    model->loadImageToInput(eyePatch);
    model->runInference();
}
