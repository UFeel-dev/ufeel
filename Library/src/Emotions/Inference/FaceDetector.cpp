#include "Emotions/Inference/FaceDetector.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

YuNetFaceDetector::YuNetFaceDetector(
    const std::string& model_path, float score_threshold, float nms_threshold, int top_k
)
{
    detector_ = cv::FaceDetectorYN::create(
        model_path,
        "",
        cv::Size(320, 320),
        score_threshold,
        nms_threshold,
        top_k,
        cv::dnn::DNN_BACKEND_OPENCV,
        cv::dnn::DNN_TARGET_CPU
    );

    if (!detector_)
    {
        throw std::runtime_error("Failed to create YuNet detector");
    }
}

auto YuNetFaceDetector::detect(
    const cv::Mat& image
) -> std::optional<FaceDetection>
{
    if (image.empty())
    {
        return std::nullopt;
    }

    detector_->setInputSize(cv::Size(image.cols, image.rows));

    cv::Mat faces;

    detector_->detect(image, faces);

    if (faces.empty())
    {
        return std::nullopt;
    }

    int best_idx = 0;
    float best_area = 0.0f;

    for (int i = 0; i < faces.rows; ++i)
    {
        const float w = faces.at<float>(i, 2);
        const float h = faces.at<float>(i, 3);

        const float area = w * h;

        if (area > best_area)
        {
            best_area = area;
            best_idx = i;
        }
    }

    FaceDetection detection;

    detection.box =
        cv::Rect2f(
            faces.at<float>(best_idx, 0),
            faces.at<float>(best_idx, 1),
            faces.at<float>(best_idx, 2),
            faces.at<float>(best_idx, 3)
        );

    detection.landmarks =
    {
        cv::Point2f(
            faces.at<float>(best_idx, 6),
            faces.at<float>(best_idx, 7)
        ),

        cv::Point2f(
            faces.at<float>(best_idx, 4),
            faces.at<float>(best_idx, 5)
        ),

        cv::Point2f(
            faces.at<float>(best_idx, 8),
            faces.at<float>(best_idx, 9)
        ),

        cv::Point2f(
            faces.at<float>(best_idx, 12),
            faces.at<float>(best_idx, 13)
        ),

        cv::Point2f(
            faces.at<float>(best_idx, 10),
            faces.at<float>(best_idx, 11)
        )
    };

    return detection;
}
