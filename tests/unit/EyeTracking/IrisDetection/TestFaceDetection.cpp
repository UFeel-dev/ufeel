#include "EyeTracking/IrisDetection/FaceDetection.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

class TestFaceDetection : public ::testing::Test
{
protected:

    void SetUp() override
    {
#ifndef MODEL_DIR_PATH
        FAIL() << "MODEL_DIR_PATH is not defined.";
        return;
#else
        modelDir_ = MODEL_DIR_PATH;
#endif

        detector_ = std::make_unique<my::FaceDetection>(modelDir_);
    }

    auto createImage(
        int width = 640, int height = 480, int type = CV_8UC3
    ) const -> cv::Mat
    {
        return cv::Mat(height, width, type, cv::Scalar(100, 150, 200, 255));
    }

    std::string modelDir_;
    std::unique_ptr<my::FaceDetection> detector_;
};

TEST_F(
    TestFaceDetection, ConstructorSucceedsWithValidModel
)
{
    EXPECT_NE(detector_, nullptr);
}
