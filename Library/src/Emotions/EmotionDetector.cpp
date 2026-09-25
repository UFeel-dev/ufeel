/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** EmotionDetector
*/

#include "Emotions/EmotionDetector.hpp"

EmotionDetector::EmotionDetector()
{
    std::string modelPath = "models/GiMeFive.pt";
    try {
        net_ = torch::jit::load(modelPath);
    } catch (const c10::Error& e) {
        std::cerr << "error loading the model: {" << modelPath << "}" << '\n';
    }

    faceDetector_ = cv::FaceDetectorYN::create(
        "models/face_detection_yunet_2023mar.onnx",
        "",
        cv::Size(320, 320),
        0.9F,
        0.3F,
        5000,
        cv::dnn::DNN_BACKEND_OPENCV,
        cv::dnn::DNN_TARGET_CPU
    );

    if (faceDetector_.empty()) {
        std::cerr << "FaceDetector is EMPTY" << '\n';
    }
    std::cout << "Success Emotion Detector Constructor" << '\n';

    std::cout << "FaceDetector created" << '\n';
    cv::Mat dummy(320, 320, CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat faces;

    try
    {
        faceDetector_->detect(dummy, faces);
        std::cout << "Detect OK" << '\n';
    }
    catch (const cv::Exception& e)
    {
        std::cerr << "Detect failed: " << e.what() << '\n';
    }
}

EmotionDetector::~EmotionDetector()
{
    close();
}

void EmotionDetector::toggleEmotionDetection(bool state)
{
    processEnable_ = state;
    std::cout << "[EmotionDetector] Emotion detection " << (state ? "enabled" : "disabled") << '\n';
}

cv::Mat EmotionDetector::preProcess(const cv::Mat &face)
{
    const float mean[3] { 0.485F, 0.456F, 0.406F };
    const float std[3] { 0.229F, 0.224F, 0.225F };
    cv::Mat resized;
    cv::Mat rgb;
    cv::Mat gray;
    cv::Mat gray3;
    std::vector<cv::Mat> ch(3);

    cv::resize(face, resized, cv::Size(64,64));
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    cv::cvtColor(rgb, gray, cv::COLOR_RGB2GRAY);
    cv::merge(std::vector<cv::Mat>{gray, gray, gray}, gray3);
    gray3.convertTo(gray3, CV_32FC3, 1.0 / 255.0);
    cv::split(gray3, ch);
    for (size_t i = 0; i < 3; i++) {
        ch[i] = (ch[i] - mean[i]) / std[i];
    }
    cv::merge(ch, gray3);
    return gray3;
}

std::vector<float> EmotionDetector::processFace(const cv::Mat &face)
{
    if (face.empty()) {
        std::cerr << "[EmotionDetector] processFace() got EMPTY face" << '\n';
        return {};
    }

    std::vector<torch::jit::IValue> inputs;
    float sum = 0.0F;
    float neutralFactor = 0.001F;
    float total = 0.0F;

    cv::Mat input = preProcess(face);
    cv::Mat blob = cv::dnn::blobFromImage(input);

    torch::Tensor tensor = torch::from_blob(
        blob.ptr<float>(),
        {1, 3, 64, 64},
        torch::kFloat32
    ).clone();


    inputs.emplace_back(tensor);

    at::Tensor output = net_.forward(inputs).toTensor();
    std::vector<float> scores(output.data_ptr<float>(), output.data_ptr<float>() + output.numel());

    float maxLogit = *std::max_element(scores.begin(), scores.end());
    for (auto &s : scores) {
        s = std::exp(s - maxLogit);
        sum += s;
    }
    for (auto &s : scores) {
        s /= sum;
    }

    scores[4] *= neutralFactor;
    for (auto s : scores) {
        total += s;
    }
    for (auto &s : scores) {
        s /= total;
    }

    return scores;
}

std::map<std::string,float> EmotionDetector::process(const cv::Mat &image)
{
    if (image.empty()) {
        std::cerr << "[EmotionDetector] process() got EMPTY image" << '\n';
        return {};
    }

    cv::Mat resized;
    cv::Mat inputImage = image.clone();
    const cv::Size yunetSize(320,320);
    cv::Mat faces;

    cv::resize(inputImage, resized, yunetSize);
    faceDetector_->setInputSize(resized.size());
    faceDetector_->detect(resized, faces);

    if (faces.empty()) {
        return {};
    }
    float scaleX = static_cast<float>(image.cols) / static_cast<float>(yunetSize.width);
    float scaleY = static_cast<float>(image.rows) / static_cast<float>(yunetSize.height);

    for (int i = 0; i < faces.rows; i++) {
        float conf = faces.at<float>(i, 14);
        if (conf < 0.9F) {
            continue;
        }
        cv::Rect faceRect(
            static_cast<int>(faces.at<float>(i, 0) * scaleX),
            static_cast<int>(faces.at<float>(i, 1) * scaleY),
            static_cast<int>(faces.at<float>(i, 2) * scaleX),
            static_cast<int>(faces.at<float>(i, 3) * scaleY)
        );
        faceRect &= cv::Rect(0, 0, image.cols, image.rows);
        std::vector<float> scores = processFace(inputImage(faceRect));
        std::map<std::string, float> labeled;
        for(size_t j = 0; j < classLabels_.size(); j++) {
            labeled[classLabels_[j]] = scores[j];
        }
        return labeled;
    }
    return {};
}

void EmotionDetector::close()
{
}