/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** EmotionDetector
*/

#ifndef EMOTIONS_EMOTIONDETECTOR_HPP
    #define EMOTIONS_EMOTIONDETECTOR_HPP

#include <filesystem>
#include <opencv2/opencv.hpp>
#include <string>
#include <torch/script.h>
#include <vector>

class EmotionDetector
{
    public:
        EmotionDetector();
        ~EmotionDetector();

        EmotionDetector(const EmotionDetector&) = delete;
        auto operator=(const EmotionDetector&) -> EmotionDetector& = delete;

        EmotionDetector(EmotionDetector&&) = delete;
        auto operator=(EmotionDetector&&) -> EmotionDetector& = delete;

        void toggleEmotionDetection(bool state);
        auto process(const cv::Mat &image) -> std::map<std::string,float>;
        void close();

    private:
        static auto preProcess(const cv::Mat &face) -> cv::Mat;
        auto processFace(const cv::Mat &face) -> std::vector<float>;

        bool processEnable_ = false;

        torch::jit::script::Module net_;
        cv::Ptr<cv::FaceDetectorYN> faceDetector_;

        const std::vector<std::string> classLabels_ {
            "happiness", "surprise", "sadness", "anger", "neutral", "fear"
        };
};

#endif // EMOTIONS_EMOTIONDETECTOR_HPP 
