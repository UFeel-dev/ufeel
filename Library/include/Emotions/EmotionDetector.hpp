/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** EmotionDetector
*/

#ifndef EMOTIONDETECTOR_HPP_
    #define EMOTIONDETECTOR_HPP_

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
        EmotionDetector& operator=(const EmotionDetector&) = delete;

        EmotionDetector(EmotionDetector&&) = delete;
        EmotionDetector& operator=(EmotionDetector&&) = delete;

        void toggleEmotionDetection(bool state);
        std::map<std::string,float> process(const cv::Mat &image);
        void close();

    private:
        static cv::Mat preProcess(const cv::Mat &face);
        std::vector<float> processFace(const cv::Mat &face);

        bool processEnable_ = false;

        torch::jit::script::Module net_;
        cv::Ptr<cv::FaceDetectorYN> faceDetector_;

        const std::vector<std::string> classLabels_ {
            "happiness", "surprise", "sadness", "anger", "neutral", "fear"
        };
};

#endif /* !EMOTIONDETECTOR_HPP_ */
