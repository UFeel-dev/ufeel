#include "EyeTracking/IrisDetection/FaceLandmark.hpp"
#include <iostream>


#define FACE_LANDMARKS 468
/*
Helper function
*/
auto __isIndexValid(int idx) -> bool {
    if (idx < 0 || idx >= FACE_LANDMARKS) {
        std::cerr << "Index " << idx << " is out of range (" \
        << FACE_LANDMARKS << ")." << std::endl;
        return false;
    }
    return true;
}


my::FaceLandmark::FaceLandmark(std::string modelPath):
    FaceDetection(modelPath),
    m_landmarkModel(modelPath + std::string("/face_landmark.tflite"))
    {}


void my::FaceLandmark::runInference() {
    FaceDetection::runInference();
    auto roi = FaceDetection::getFaceRoi();
    if (roi.empty()) return;

    auto face = FaceDetection::cropFrame(roi);
    m_landmarkModel.loadImageToInput(face);
    m_landmarkModel.runInference();
}


auto my::FaceLandmark::getFaceLandmarkAt(int index) const -> cv::Point {
    if (__isIndexValid(index)) {
        auto roi = FaceDetection::getFaceRoi();

        float _x = m_landmarkModel.getOutputData()[index * 3];
        float _y = m_landmarkModel.getOutputData()[index * 3 + 1];

        int x = (int)(_x / m_landmarkModel.getInputShape()[2] * roi.width) + roi.x;
        int y = (int)(_y / m_landmarkModel.getInputShape()[1] * roi.height) + roi.y;

        return cv::Point(x,y);
    }
    return cv::Point();
}


auto my::FaceLandmark::getAllFaceLandmarks() const -> std::vector<cv::Point> {
    if (FaceDetection::getFaceRoi().empty())
        return std::vector<cv::Point>();

    std::vector<cv::Point> landmarks(FACE_LANDMARKS);
    for (int i = 0; i < FACE_LANDMARKS; ++i) {
        landmarks[i] = getFaceLandmarkAt(i);
    }
    return landmarks;
}


auto my::FaceLandmark::loadOutput(int index) const -> std::vector<float> {
    return m_landmarkModel.loadOutput();
}