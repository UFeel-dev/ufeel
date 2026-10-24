#include "EyeTracking/IrisDetection/ModelLoader.hpp"

#include "opencv2/imgproc.hpp"
#include "tensorflow/lite/core/c/c_api_types.h"
#include "tensorflow/lite/interpreter_builder.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model_builder.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr float INPUT_NORM_MEAN = 127.5f;
constexpr float INPUT_NORM_STD = 127.5f;

} // namespace

my::ModelLoader::ModelLoader(
    const std::string& modelPath
)
{
    loadModel(modelPath.c_str());
    buildInterpreter();
    allocateTensors();
    fillInputTensors();
    fillOutputTensors();

    inputLoads_.resize(inputs_.size(), false);
}

auto my::ModelLoader::getInputShape(
    int index
) const -> std::vector<int>
{
    if (isIndexValid(index, 'i'))
    {
        return inputs_.at(static_cast<std::size_t>(index)).dims;
    }

    return {};
}

auto my::ModelLoader::getInputData(
    int index
) const -> float*
{
    if (isIndexValid(index, 'i'))
    {
        return inputs_.at(static_cast<std::size_t>(index)).data;
    }

    return nullptr;
}

auto my::ModelLoader::getInputSize(
    int index
) const -> size_t
{
    if (isIndexValid(index, 'i'))
    {
        return inputs_.at(static_cast<std::size_t>(index)).bytes;
    }

    return 0;
}

auto my::ModelLoader::getNumberOfInputs() const -> int
{
    return static_cast<int>(inputs_.size());
}

auto my::ModelLoader::getOutputShape(
    int index
) const -> std::vector<int>
{
    if (isIndexValid(index, 'o'))
    {
        return outputs_.at(static_cast<std::size_t>(index)).dims;
    }

    return {};
}

auto my::ModelLoader::getOutputData(
    int index
) const -> float*
{
    if (isIndexValid(index, 'o'))
    {
        return outputs_.at(static_cast<std::size_t>(index)).data;
    }

    return nullptr;
}

auto my::ModelLoader::getOutputSize(
    int index
) const -> size_t
{
    if (isIndexValid(index, 'o'))
    {
        return outputs_.at(static_cast<std::size_t>(index)).bytes;
    }

    return 0;
}

auto my::ModelLoader::getNumberOfOutputs() const -> int
{
    return static_cast<int>(outputs_.size());
}

void my::ModelLoader::loadImageToInput(
    const cv::Mat& inputImage, int index
)
{
    if (!isIndexValid(index, 'i'))
    {
        throw std::out_of_range("Invalid model input index.");
    }

    auto const resizedImage = preprocessImage(inputImage, index);
    auto const imageBytes = resizedImage.total() * resizedImage.elemSize();

    if (imageBytes != getInputSize(index))
    {
        throw std::runtime_error("Preprocessed image size does not match the model input size.");
    }

    loadBytesToInput(resizedImage.data, index);
}

void my::ModelLoader::loadBytesToInput(
    const void* data, int index
)
{
    if (!isIndexValid(index, 'i'))
    {
        throw std::out_of_range("Invalid model input index.");
    }

    auto const inputIndex = static_cast<std::size_t>(index);
    auto const inputSize = inputs_.at(inputIndex).bytes;
    auto* inputData = inputs_.at(inputIndex).data;

    if (data == nullptr || inputData == nullptr || inputSize == 0)
    {
        throw std::invalid_argument("Model input data must not be null or empty.");
    }

    std::memcpy(inputData, data, inputSize);
    inputLoads_.at(inputIndex) = true;
}

void my::ModelLoader::runInference()
{
    inputChecker();

    if (interpreter_->Invoke() != kTfLiteOk)
    {
        throw std::runtime_error("TensorFlow Lite inference failed.");
    }
}

auto my::ModelLoader::loadOutput(
    int index
) const -> std::vector<float>
{
    if (!isIndexValid(index, 'o'))
    {
        return {};
    }

    auto const outputIndex = static_cast<std::size_t>(index);
    auto const& output = outputs_.at(outputIndex);

    if (output.data == nullptr || output.bytes % sizeof(float) != 0)
    {
        return {};
    }

    auto const floatCount = output.bytes / sizeof(float);
    std::vector<float> inference(floatCount);

    if (output.bytes > 0)
    {
        std::memcpy(inference.data(), output.data, output.bytes);
    }

    return inference;
}

void my::ModelLoader::loadModel(
    const char* modelPath
)
{
    model_ = tflite::FlatBufferModel::BuildFromFile(modelPath);
    if (model_ == nullptr)
    {
        throw std::runtime_error(std::string("Failed to load TensorFlow Lite model: ") + modelPath);
    }
}

void my::ModelLoader::buildInterpreter(
    int numThreads
)
{
    tflite::ops::builtin::BuiltinOpResolver const resolver;

    if (tflite::InterpreterBuilder(*model_, resolver)(&interpreter_) != kTfLiteOk ||
        interpreter_ == nullptr)
    {
        throw std::runtime_error("Failed to build TensorFlow Lite interpreter.");
    }

    interpreter_->SetNumThreads(numThreads);
}

void my::ModelLoader::allocateTensors()
{
    if (interpreter_ == nullptr || interpreter_->AllocateTensors() != kTfLiteOk)
    {
        throw std::runtime_error("Failed to allocate TensorFlow Lite tensors.");
    }
}

void my::ModelLoader::fillInputTensors()
{
    for (auto const tensorIndex : interpreter_->inputs())
    {
        auto const* tensor = interpreter_->tensor(tensorIndex);
        if (tensor == nullptr || tensor->data.f == nullptr || tensor->dims == nullptr)
        {
            throw std::runtime_error("Invalid TensorFlow Lite input tensor.");
        }

        inputs_.emplace_back(
            tensor->data.f, tensor->bytes, tensor->dims->data, tensor->dims->size
        );
    }

    if (inputs_.empty())
    {
        throw std::runtime_error("The TensorFlow Lite model has no input tensors.");
    }
}

void my::ModelLoader::fillOutputTensors()
{
    for (auto const tensorIndex : interpreter_->outputs())
    {
        auto const* tensor = interpreter_->tensor(tensorIndex);
        if (tensor == nullptr || tensor->data.f == nullptr || tensor->dims == nullptr)
        {
            throw std::runtime_error("Invalid TensorFlow Lite output tensor.");
        }

        outputs_.emplace_back(
            tensor->data.f, tensor->bytes, tensor->dims->data, tensor->dims->size
        );
    }

    if (outputs_.empty())
    {
        throw std::runtime_error("The TensorFlow Lite model has no output tensors.");
    }
}

auto my::ModelLoader::isIndexValid(
    int index, char type
) const -> bool
{
    std::size_t size = 0;

    if (type == 'i')
    {
        size = inputs_.size();
    }
    else if (type == 'o')
    {
        size = outputs_.size();
    }
    else
    {
        return false;
    }

    if (index < 0 || std::cmp_greater_equal(index, size))
    {
        std::cerr << "Tensor index " << index << " is out of range (" << size << ").\n";
        return false;
    }

    return true;
}

auto my::ModelLoader::isAllInputsLoaded() const -> bool
{
    return std::ranges::find(inputLoads_, false) == inputLoads_.end();
}

void my::ModelLoader::inputChecker()
{
    if (!isAllInputsLoaded())
    {
        std::string message = "Model inputs have not all been loaded:";
        for (std::size_t index = 0; index < inputLoads_.size(); ++index)
        {
            if (!inputLoads_.at(index))
            {
                message += " " + std::to_string(index);
            }
        }

        throw std::runtime_error(message);
    }

    std::ranges::fill(inputLoads_, false);
}

auto my::ModelLoader::preprocessImage(
    const cv::Mat& input, int index
) const -> cv::Mat
{
    if (input.empty())
    {
        throw std::invalid_argument("Cannot preprocess an empty image.");
    }

    auto const inputShape = getInputShape(index);
    if (inputShape.size() < 3 || inputShape.at(1) <= 0 || inputShape.at(2) <= 0 ||
        (inputShape.size() >= 4 && inputShape.at(3) != 3))
    {
        throw std::runtime_error("Unsupported model image input shape.");
    }

    auto output = convertToRGB(input);

    cv::resize(output, output, cv::Size(inputShape.at(2), inputShape.at(1)));

    output.convertTo(output, CV_32FC3, 1.0 / INPUT_NORM_STD, -INPUT_NORM_MEAN / INPUT_NORM_STD);

    return output;
}

auto my::ModelLoader::convertToRGB(
    const cv::Mat& input
) -> cv::Mat
{
    if (input.empty())
    {
        throw std::invalid_argument("Cannot convert an empty image.");
    }

    cv::Mat output;

    if (input.type() == CV_8UC3)
    {
        cv::cvtColor(input, output, cv::COLOR_BGR2RGB);
    }
    else if (input.type() == CV_8UC4)
    {
        cv::cvtColor(input, output, cv::COLOR_BGRA2RGB);
    }
    else
    {
        throw std::invalid_argument("Image must have type CV_8UC3 or CV_8UC4.");
    }

    return output;
}
