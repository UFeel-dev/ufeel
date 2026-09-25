#ifndef EYETRACKING_IRISDETECTION_FACEDETECTION_HPP
#define EYETRACKING_IRISDETECTION_FACEDETECTION_HPP

#include "DetectionPostProcess.hpp"
#include "ModelLoader.hpp"

namespace my {

    /*
    A model wrapper to use Mediapipe Face Detector.
    This class is non-copyable.
    */
    class FaceDetection : public my::ModelLoader {
        public:
            /*
            Users MUST provide the FOLDER contain face_detection_short.tflite, NOT THE FILE itself.
            */
            explicit FaceDetection(std::string modelPath);
            ~FaceDetection() override = default;

            FaceDetection(const FaceDetection&) = delete;
            auto operator=(const FaceDetection&) -> FaceDetection& = delete;
            FaceDetection(FaceDetection&&) noexcept = default;
            auto operator=(FaceDetection&&) noexcept -> FaceDetection& = default;

            // Bring non-virtual convenience methods from ModelLoader into scope
            using my::ModelLoader::loadImageToInput;
            using my::ModelLoader::loadBytesToInput;
            using my::ModelLoader::loadOutput;

            /*
            Get access to original input image
            */
            [[nodiscard]]
            auto getOriginalImage() const -> cv::Mat;

            /*
            Get the regressor result (first output tensor).
            */
            [[nodiscard]]
            auto getFaceRegressor() const -> std::vector<float>;

            /*
            Get the classificator result (second output tensor).
            */  
            [[nodiscard]]       
            auto getFaceClassificator() const -> std::vector<float>;

            /*
            Get the position of the HIGHEST CONFIDENT face
            (Note: the position is relative to the image passed to InputTensor(0))
            */
            [[nodiscard]]
            virtual auto getFaceRoi() const -> cv::Rect;

            /*
            Override function from ModelLoader.
            (Note: index does not matter, the model always load to InputTensor(0))
            */
            void loadImageToInput(const cv::Mat& inputImage, int index) override;       

            /*
            Override function from ModelLoader.
            Can only run when all input tensors have been loaded.
            */
            void runInference() override;

            /*
            Crop input frame at roi (padding if need)
            */
            [[nodiscard]]
            auto cropFrame(const cv::Rect& roi) const -> cv::Mat;


        private:

            /*       
            Convert Detection box back to original size
            */
            [[nodiscard]]
            auto calculateRoiFromDetection(const Detection& detection) const -> cv::Rect;

            /*
            Help getting Region of Interest from model outputs
            */
            DetectionPostProcess m_postProcessor;

            /*
            Save some informations
            */
            cv::Mat m_originImage;
            cv::Rect m_roi;
    };
}
#endif // EYETRACKING_IRISDETECTION_FACEDETECTION_HPP