#include "Emotions/Inference/InferencePipeline.hpp"

InferencePipeline::InferencePipeline(
    EmotionModel model,
    torch::Device device,
    std::shared_ptr<MTCNNCropTransform> face_crop,
    int img_size
)
    : model_(std::move(model))
    , device_(device)
    , faceCrop_(std::move(face_crop))
    , imgSize_(img_size)
{
    model_->to(device_);
    model_->eval();
}

auto InferencePipeline::preprocess(
    const cv::Mat& image
) -> torch::Tensor
{
    cv::Mat processed = image;

    // if (faceCrop_)
    // {
    //     processed = (*faceCrop_)(processed);
    // }

    if (processed.empty())
    {
        throw std::runtime_error("Preprocessing produced an empty image");
    }

    cv::Mat resized;

    cv::resize(processed, resized, cv::Size(imgSize_, imgSize_), 0.0, 0.0, cv::INTER_LINEAR);

    cv::Mat rgb;

    if (resized.channels() == 3)
    {
        cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    }
    else if (resized.channels() == 4)
    {
        cv::cvtColor(resized, rgb, cv::COLOR_BGRA2RGB);
    }
    else
    {
        throw std::runtime_error("Expected 3 or 4 channel image");
    }

    rgb.convertTo(rgb, CV_32FC3, 1.0 / 255.0);

    auto tensor = torch::from_blob(
        rgb.data, {rgb.rows, rgb.cols, 3}, torch::TensorOptions().dtype(torch::kFloat32)
    );

    tensor = tensor.permute({2, 0, 1}).contiguous().clone().unsqueeze(0);

    const auto mean =
        torch::tensor({0.485f, 0.456f, 0.406f}, torch::TensorOptions().dtype(torch::kFloat32))
            .view({1, 3, 1, 1});

    const auto std =
        torch::tensor({0.229f, 0.224f, 0.225f}, torch::TensorOptions().dtype(torch::kFloat32))
            .view({1, 3, 1, 1});

    tensor = (tensor - mean) / std;

    return tensor;
}

void InferencePipeline::set_calibration(
    const HeadCalibration& calibration
)
{
    calibration_ = std::make_unique<HeadCalibration>(calibration);
}

auto InferencePipeline::predict(
    const cv::Mat& image, int topk
) -> PredictionResult
{
    (void)topk;

    torch::NoGradGuard no_grad;
    cv::Mat rgb;
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);

    auto x = preprocess(image).to(device_);

    auto logits = model_->forward(x);

    PredictionResult result;

    for (const auto& [name, value] : logits)
    {
        auto probability = torch::sigmoid(value);

        result.heads[name] = probability[0].item<float>();
    }

    if (calibration_)
    {
        result.binary = calibration_->apply(result.heads);
    }

    return result;
}
