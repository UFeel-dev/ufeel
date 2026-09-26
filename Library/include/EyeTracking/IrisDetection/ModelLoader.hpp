#ifndef EYETRACKING_IRISDETECTION_MODELLOADER_HPP
#define EYETRACKING_IRISDETECTION_MODELLOADER_HPP

#include <memory>
#include <string>
#include <vector>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/model.h"

namespace my {

    template <class T>
    using Matrix = std::vector<std::vector<T>>;

    /*
    A tensor wrapper to save information of tflite tensors.
    Attributes:
        data: a float pointer to tensor data
        bytes: size of data in bytes
        dims: shape of data tensor
    */
    struct TensorWrapper {
        float* data;
        size_t bytes;
        std::vector<int> dims;

        TensorWrapper(float* t_data, size_t t_bytes, int* t_dims, int t_dimSize): 
            data(t_data), bytes(t_bytes), dims(t_dims, t_dims + t_dimSize) {}
    };

    /*
    A model wrapper to simplify the procedure of using tflite's models.
    This class is non-copyable.
    */
    class ModelLoader {
        public:
            /*
            Constructor from a .tflite file
            Parameters:
                modelPath: path to .tflite
            */
            explicit ModelLoader(std::string modelPath);
            ModelLoader(const ModelLoader& other) = delete;
            auto operator=(const ModelLoader& other) -> ModelLoader& = delete;
            ModelLoader(ModelLoader&&) = delete;
            auto operator=(ModelLoader&&) -> ModelLoader& = delete;
            virtual ~ModelLoader() = default;

            /*
            Get shape of input tensor at index.
            (Note: A model can have multiple inputs)
            Parameters:
                index: index of input tensor
            */
            [[nodiscard]] auto getInputShape(int index = 0) const -> std::vector<int>;

            /*
            Get the pointer to the data of input tensor at index.
            (Note: A model can have multiple inputs)
            Parameters:
                index: index of input tensor
            */
            [[nodiscard]] auto getInputData(int index = 0) const -> float*;

            /*
            Get size in bytes of input tensor at index.
            (Note: A model can have multiple inputs)
            Parameters:
                index: index of input tensor
            */
            [[nodiscard]] auto getInputSize(int index = 0) const -> size_t;

            /*
            Get number of inputs needed to run inference. 
            */
            [[nodiscard]] auto getNumberOfInputs() const -> int;

            /*
            Get shape of output tensor at index.
            (Note: A model can have multiple outputs)
            Parameters:
                index: index of output tensor
            */     
            [[nodiscard]] auto getOutputShape(int index = 0) const -> std::vector<int>;

            /*
            Get the pointer to the data of output tensor at index.
            (Note: A model can have multiple outputs)
            Parameters:
                index: index of output tensor
            */
            [[nodiscard]] auto getOutputData(int index = 0) const -> float*;

            /*
            Get size in bytes of output tensor at index.
            (Note: A model can have multiple outputs)
            Parameters:
                index: index of output tensor
            */
            [[nodiscard]] auto getOutputSize(int index = 0) const -> size_t;

            /*
            Get number of outputs from inference.
            */
            [[nodiscard]] auto getNumberOfOutputs() const -> int;

            /*
            Load image (BGR format) to model at index 
            (Note: Only support image of type CV_8UC3 and CV_8UC4)
            */
            virtual void loadImageToInput(const cv::Mat& inputImage, int index);

            /*
            Load byte data to model at index
            */
            virtual void loadBytesToInput(const void* data, int index);

            /*
            Run inference on the inputs.
            Can only run when all input tensors have been loaded.
            */
            virtual void runInference();

            /*
            A vector contains output data at index.
            Its shape is flattened from getOutputShape(index)
            */
            [[nodiscard]] virtual auto loadOutput(int index) const -> std::vector<float>;

            // Non-virtual convenience overloads (provides default index = 0)
            void loadImageToInput(const cv::Mat& inputImage) {
                loadImageToInput(inputImage, 0);
            }

            void loadBytesToInput(const void* data) {
                loadBytesToInput(data, 0);
            }

            [[nodiscard]] auto loadOutput() const -> std::vector<float> {
                return loadOutput(0);
            }

        private:
            /*
            Constructor helper functions
            */
            void loadModel(const char* modelPath);
            void buildInterpreter(int numThreads = -1);
            void allocateTensors();           
            void fillInputTensors();
            void fillOutputTensors();

            /*
            Check if index is valid for input and output tensor
            */
            [[nodiscard]] auto isIndexValid(int index, char c = 'i') const -> bool;

            /*
            Check if all inputs have been loaded
            */
            [[nodiscard]] auto isAllInputsLoaded() const -> bool;

            /*
            Process input loads before run inference
            */
            void inputChecker();

            /*
            Convert image to float and resize to getInputShape(idx)
            */
            [[nodiscard]] auto preprocessImage(const cv::Mat& in, int idx) const -> cv::Mat;

            /*
            Convert image of type CV_8UC3 or CV_8UC4 to RGB format
            */
            [[nodiscard]] auto convertToRGB(const cv::Mat& in) const -> cv::Mat;


        // next block of private:
            /*
            Information of input tensors
            */
            std::vector<TensorWrapper> m_inputs;

            /*
            Information of output tensors
            */
            std::vector<TensorWrapper> m_outputs;

            /*
            TFLite core
            */
            std::unique_ptr<tflite::FlatBufferModel> m_model;

            /*
            TFLite core
            */           
            std::unique_ptr<tflite::Interpreter> m_interpreter;

            /*
            Tracking inputs loaded
            */
            std::vector<bool> m_inputLoads;
    };
};

#endif // EYETRACKING_IRISDETECTION_MODELLOADER_HPP