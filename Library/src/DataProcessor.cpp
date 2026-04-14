/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** DataProcessor
*/

#include "DataProcessor.hpp"

DataProcessor::DataProcessor(bool calibration) :
    cap_(0),
    counter_(0),
    freq_(5),
    scaleFactor_(1.3),
    calibration_(calibration)
{
    if (!cap_.isOpened())
    {
        std::cerr << "[ERROR] Cannot open camera capture_id=0\n";
    }
    // # if not calibration:

    emotionDetector_ = std::make_unique<EmotionDetector>();
    eyeTrackingDetector_ = std::make_unique<EyeTrackingDetector>();
    speechToTextDetector_ = std::make_unique<SpeechToTextDetector>();
    // heartRateSensorDetector_ = std::make_unique<HeartRateSensorDetector>();
}

cv::Mat DataProcessor::getFrame()
{
    cap_ >> frame_;
    return frame_;
}

std::map<std::string, float> DataProcessor::processEmotion()
{
    cap_ >> frame_;
    if (frame_.empty())
        return {}; // TODO: add default value for all of this

    return emotionDetector_->process(frame_);
}

std::map<std::string, bool> DataProcessor::processEyeTracking()
{
    cap_ >> frame_;
    if (frame_.empty())
        return {};

    return eyeTrackingDetector_->process(frame_);
}

std::string DataProcessor::processSpeechToText()
{
    return speechToTextDetector_->process();
}

// int DataProcessor::processHeartRateSensor()
// {
    // return heartRateSensorDetector_->process();
//     return 0;
// }


// void DataProcessor::process()
// {
//     if (!cap_.isOpened())
//         return;

//     cv::Mat frame;

//     while (cap_.isOpened())
//     {
//         bool ret = cap_.read(frame);
//         if (!ret || frame.empty())
//             break;

//         cv::flip(frame, frame, 1);

        // emotionDetector_->process(frame, counter_, show_window_);
//         // eye_tracker_->process(frame, calibration_, show_window_);
//         // speech_to_text_->process(frame);
//         // heart_rate_sensor_->process(frame);

//         if (showWindow_)
//         {
//             cv::Mat resized_frame;
//             cv::resize(
//                 frame,
//                 resized_frame,
//                 cv::Size(),
//                 scaleFactor_,
//                 scaleFactor_,
//                 cv::INTER_LINEAR
//             );

//             cv::imshow("Combined Output", resized_frame);

//             int key = cv::waitKey(1);
//             if ((key & 0xFF) == 'q')
//                 break;
//         }
//         counter_ = (counter_ + 1) % freq_;
//     }
// }

DataProcessor::~DataProcessor()
{
    if (cap_.isOpened())
        cap_.release();

    emotionDetector_->close();
    eyeTrackingDetector_->close();
    // speechToTextDetector_->close();
    // heartRateSensorDetector_->close();
}
