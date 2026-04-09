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
        std::cerr << "error loading the model: {" << modelPath << "}" << std::endl;
    }

    faceDetector_ = cv::FaceDetectorYN::create(
        "models/face_detection_yunet_2023mar.onnx",
        "",
        cv::Size(320, 320),
        0.9,
        0.3,
        5000,
        cv::dnn::DNN_BACKEND_OPENCV,
        cv::dnn::DNN_TARGET_CPU
    );

    if (faceDetector_.empty()) {
        std::cerr << "FaceDetector is EMPTY" << std::endl;
    }
    std::cout << "Success Emotion Detector Constructor" << std::endl;

    std::cout << "FaceDetector created" << std::endl;
    cv::Mat dummy(320, 320, CV_8UC3, cv::Scalar(0,0,0));
    cv::Mat faces;

    try
    {
        faceDetector_->detect(dummy, faces);
        std::cout << "Detect OK" << std::endl;
    }
    catch (const cv::Exception& e)
    {
        std::cerr << "Detect failed: " << e.what() << std::endl;
    }
}

EmotionDetector::~EmotionDetector()
{
    close();
}

void EmotionDetector::toggleEmotionDetection(bool state)
{
    processEnable_ = state;
    std::cout << "[EmotionDetector] Emotion detection " << (state ? "enabled" : "disabled") << std::endl;
}

cv::Mat EmotionDetector::preprocess(const cv::Mat &face)
{
    cv::Mat resized, rgb, gray;
    cv::resize(face, resized, cv::Size(64,64));
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
    cv::cvtColor(rgb, gray, cv::COLOR_RGB2GRAY);
    cv::Mat gray3;
    cv::merge(std::vector<cv::Mat>{gray, gray, gray}, gray3);
    gray3.convertTo(gray3, CV_32FC3, 1.0 / 255.0);
    std::vector<cv::Mat> ch(3);
    cv::split(gray3, ch);
    const float mean[3] { 0.485f, 0.456f, 0.406f };
    const float std[3] { 0.229f, 0.224f, 0.225f };
    for (int i = 0; i < 3; i++)
        ch[i] = (ch[i] - mean[i]) / std[i];
    cv::merge(ch, gray3);
    return gray3;
}

std::vector<float> EmotionDetector::processFace(const cv::Mat &face)
{
    if (face.empty()) {
        std::cerr << "[EmotionDetector] processFace() got EMPTY face" << std::endl;
        return {};
    }

    cv::Mat input = preprocess(face);
    cv::Mat blob = cv::dnn::blobFromImage(input);

    torch::Tensor tensor = torch::from_blob(
        blob.ptr<float>(),
        {1, 3, 64, 64},
        torch::kFloat32
    ).clone();

    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(tensor);

    at::Tensor output = net_.forward(inputs).toTensor();
    std::vector<float> scores(output.data_ptr<float>(), output.data_ptr<float>() + output.numel());

    float maxLogit = *std::max_element(scores.begin(), scores.end());
    float sum = 0.0f;
    for (auto &s : scores) {
        s = std::exp(s - maxLogit);
        sum += s;
    }
    for (auto &s : scores)
        s /= sum;

    float neutralFactor = 0.001f;
    scores[4] *= neutralFactor;

    float total = 0.0f;
    for (auto s : scores)
        total += s;
    for (auto &s : scores)
        s /= total;

    return scores;
}

std::map<std::string,float> EmotionDetector::process(const cv::Mat &image)
{
    if (image.empty()) {
        std::cerr << "[EmotionDetector] process() got EMPTY image" << std::endl;
        return {};
    }

    cv::Mat resized;
    cv::Mat inputImage = image.clone();
    const cv::Size yunetSize(320,320);
    cv::resize(inputImage, resized, yunetSize);
    faceDetector_->setInputSize(resized.size());

    cv::Mat faces;
    faceDetector_->detect(resized, faces);

    if (faces.empty()) return {};

    float scaleX = float(image.cols) / yunetSize.width;
    float scaleY = float(image.rows) / yunetSize.height;

    for (int i = 0; i < faces.rows; i++) {
        float conf = faces.at<float>(i, 14);
        if (conf < 0.9f) continue;
        cv::Rect faceRect(
            int(faces.at<float>(i, 0) * scaleX),
            int(faces.at<float>(i, 1) * scaleY),
            int(faces.at<float>(i, 2) * scaleX),
            int(faces.at<float>(i, 3) * scaleY)
        );
        faceRect &= cv::Rect(0, 0, image.cols, image.rows);
        std::vector<float> scores = processFace(inputImage(faceRect));
        std::map<std::string, float> labeled;
        for(size_t j = 0; j < classLabels_.size(); j++)
            labeled[classLabels_[j]] = scores[j];
        return labeled;
    }
    return {};
}

void EmotionDetector::close()
{
}