#ifndef EYETRACKING_IRISDETECTION_IRISLANDMARK_HPP
#define EYETRACKING_IRISDETECTION_IRISLANDMARK_HPP

#include "FaceLandmark.hpp"
#include <bitset>

namespace my {
    /*
    const std::bitset<468> eyeAnnotations("000000010000000000000011111111110100000000\
    000000000000001000000000000000000000000000000000000000000000000000001111000111110\
    000001010010000000001110000000111011111010000000001000000000000000110000000000000\
    000000000000000001111110111111000000000111110100111111111101000000000000000000000\
    010000000000000000000000000000000000000000000000000000111100011111000000101001000\
    000000111000001110111110100000001000000000000001100000000000000000000000000111111\
    011111100000000011111");
    */

    /*
    A model wrapper to use Mediapipe Iris Landmark.
    It includes the face detection and face landmark phases.  
    This class is non-copyable.
    */
    class IrisLandmark: public my::FaceLandmark {
        public:
            /*
            Users MUST provide the FOLDER contain ALL the face_detection_short.tflite, 
            face_landmark.tflite and iris_landmark.tflite 
            */
            explicit IrisLandmark(std::string modelPath);
            ~IrisLandmark() override = default; 

            IrisLandmark(const IrisLandmark&) = delete;
            auto operator=(const IrisLandmark&) -> IrisLandmark& = delete;
            IrisLandmark(IrisLandmark&&) noexcept = default;
            auto operator=(IrisLandmark&&) noexcept -> IrisLandmark& = default;

            /*
            Override function from FaceLandmark
            */
            void runInference() override;

            /*
            Get an eye/iris landmark from output.
            If isIris == true: index must be in range 0-4
            else index must be in range 0-70
            The position is relative to the input image at InputTensor(0)
            */
            [[nodiscard]]
            virtual auto getEyeLandmarkAt(int index, bool isLeftEye, bool isIris) const -> cv::Point;

            /*
            Get all eye/iris landmarks from output.
            The positions is relative to the input image at InputTensor(0)
            */
            [[nodiscard]]
            virtual auto getAllEyeLandmarks(bool isLeftEye, bool isIris) const -> std::vector<cv::Point>;

            /*
            Get all landmarks from output (index = 0: Eye landmarks, index != 0: Iris landmarks)
            Each landmark is represented by x, y, z(depth), which are raw outputs from Mediapipe Iris Landmark model.
            If you want to get relative position to input image, use getAllIrisLandmarks() or getAllIrisLandmark()
            */
            
            // 1. Un-hide the base class version so callers can still do: iris.loadOutput(0)
            using my::FaceLandmark::loadOutput; 

            // 2. Add your new 2-parameter method as a standard, non-virtual function
            [[nodiscard]] auto loadOutput(int index, bool isLeftEye) const -> std::vector<float>;

            // 3. Optional: inline helper for 0 arguments
            [[nodiscard]] auto loadOutput() const -> std::vector<float> {
                return loadOutput(0, true); 
            }

            // Overrides FaceLandmark::loadOutput(int)
            [[nodiscard]] auto loadOutput(int index) const -> std::vector<float> override;

            /*
            Get eye Roi relative to input image at InputTensor(0)
            */
            [[nodiscard]] auto getEyeRoi(bool isLeftEye) const -> cv::Rect;


        private:
            /*
            Calculate Eye Roi from the first and last EyeLower2 landmark
            */
            [[nodiscard]]
            auto calculateEyeRoi(cv::Point leftMoft, cv::Point rightMost) const -> cv::Rect;

            /*
            Run inference on each eye (for multithread)
            */
            void runEyeInference(bool isLeftEye);

            ModelLoader m_leftIrisLandmarker;
            ModelLoader m_rightIrisLandmarker;

            cv::Rect m_leftEyeRoi;
            cv::Rect m_rightEyeRoi;
    };
}
#endif // EYETRACKING_IRISDETECTION_IRISLANDMARK_HPP