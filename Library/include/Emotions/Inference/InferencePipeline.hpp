#ifndef INFERENCEPIPELINE_HPP_
#define INFERENCEPIPELINE_HPP_

#include "Emotions/EmotionModel.hpp"
#include "FaceCrop.hpp"
#include "HeadCalibration.hpp"

#include <memory>
#include <string>
#include <torch/torch.h>
#include <unordered_map>
#include <vector>

struct PredictionResult
{
        std::unordered_map<std::string, float> heads;
        std::unordered_map<std::string, bool> binary;
};

class InferencePipeline
{
    public:

        InferencePipeline(
            EmotionModel model,
            torch::Device device,
            std::shared_ptr<MTCNNCropTransform> face_crop,
            int img_size
        );

        auto preprocess(const cv::Mat& image) -> torch::Tensor;
        void set_calibration(const HeadCalibration& calibration);
        auto predict(const cv::Mat& image, int topk = 3) -> PredictionResult;

    private:

        EmotionModel model_{nullptr};
        torch::Device device_;

        std::shared_ptr<MTCNNCropTransform> faceCrop_;
        int imgSize_;

        std::unique_ptr<HeadCalibration> calibration_;
};

#endif /* !INFERENCEPIPELINE_HPP_ */
