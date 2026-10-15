#include "EyeTracking/IrisDetection/ModelLoader.hpp"

#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgproc.hpp"
#include "tensorflow/lite/core/model_builder.h"
#include "tensorflow/lite/interpreter_builder.h"
#include "tensorflow/lite/kernels/register.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#define INPUT_NORM_MEAN 127.5f
#define INPUT_NORM_STD 127.5f

my::ModelLoader::ModelLoader(
    const std::string& modelPath
)
{
    loadModel(modelPath.c_str());
    buildInterpreter();
    allocateTensors();
    fillInputTensors();
    fillOutputTensors();

    m_inputLoads.resize(getNumberOfInputs(), false);
}

auto my::ModelLoader::getInputShape(
    int index
) const -> std::vector<int>
{
    if (isIndexValid(index, 'i'))
    {
        return m_inputs[index].dims;
    }

    return {};
}

auto my::ModelLoader::getInputData(
    int index
) const -> float*
{
    if (isIndexValid(index, 'i'))
    {
        return m_inputs[index].data;
    }

    return nullptr;
}

auto my::ModelLoader::getInputSize(
    int index
) const -> size_t
{
    if (isIndexValid(index, 'i'))
    {
        return m_inputs[index].bytes;
    }

    return 0;
}

auto my::ModelLoader::getNumberOfInputs() const -> int
{
    return m_inputs.size();
}

auto my::ModelLoader::getOutputShape(
    int index
) const -> std::vector<int>
{
    if (isIndexValid(index, 'o'))
    {
        return m_outputs[index].dims;
    }

    return {};
}

auto my::ModelLoader::getOutputData(
    int index
) const -> float*
{
    if (isIndexValid(index, 'o'))
    {
        return m_outputs[index].data;
    }

    return nullptr;
}

auto my::ModelLoader::getOutputSize(
    int index
) const -> size_t
{
    if (isIndexValid(index, 'o'))
    {
        return m_outputs[index].bytes;
    }

    return 0;
}

auto my::ModelLoader::getNumberOfOutputs() const -> int
{
    return m_outputs.size();
}

void my::ModelLoader::loadImageToInput(
    const cv::Mat& inputImage, int idx
)
{
    if (isIndexValid(idx, 'i'))
    {
        cv::Mat const resizedImage = preprocessImage(inputImage, idx); // Need optimize
        loadBytesToInput(resizedImage.data, idx);
    }
}

void my::ModelLoader::loadBytesToInput(
    const void* data, int idx
)
{
    if (isIndexValid(idx, 'i'))
    {
        memcpy(m_inputs[idx].data, data, m_inputs[idx].bytes);
        m_inputLoads[idx] = true;
    }
}

void my::ModelLoader::runInference()
{
    inputChecker();
    m_interpreter->Invoke(); // Tflite inference
}

auto my::ModelLoader::loadOutput(
    int index
) const -> std::vector<float>
{
    if (isIndexValid(index, 'o'))
    {
        int const sizeInByte = m_outputs[index].bytes;
        int const sizeInFloat = sizeInByte / sizeof(float);

        std::vector<float> inference(sizeInFloat);
        memcpy(inference.data(), m_outputs[index].data, sizeInByte);

        return inference;
    }
    return {};
}

//-------------------Private methods start here-------------------

void my::ModelLoader::loadModel(
    const char* modelPath
)
{
    m_model = tflite::FlatBufferModel::BuildFromFile(modelPath);
    if (m_model == nullptr)
    {
        std::cerr << "Fail to build FlatBufferModel from file: " << modelPath << '\n';
        std::exit(1);
    }
}

void my::ModelLoader::buildInterpreter(
    int numThreads
)
{
    tflite::ops::builtin::BuiltinOpResolver const resolver;

    if (tflite::InterpreterBuilder(*m_model, resolver)(&m_interpreter) != kTfLiteOk)
    {
        std::cerr << "Failed to build interpreter." << '\n';
        std::exit(1);
    }
    m_interpreter->SetNumThreads(numThreads);
}

void my::ModelLoader::allocateTensors()
{
    if (m_interpreter->AllocateTensors() != kTfLiteOk)
    {
        std::cerr << "Failed to allocate tensors." << '\n';
        std::exit(1);
    }
}

void my::ModelLoader::fillInputTensors()
{
    for (auto input : m_interpreter->inputs())
    {
        TfLiteTensor const* inputTensor = m_interpreter->tensor(input);
        TfLiteIntArray* dims = inputTensor->dims;

        m_inputs.emplace_back(inputTensor->data.f, inputTensor->bytes, dims->data, dims->size);
    }
}

void my::ModelLoader::fillOutputTensors()
{
    for (auto output : m_interpreter->outputs())
    {
        TfLiteTensor const* outputTensor = m_interpreter->tensor(output);
        TfLiteIntArray* dims = outputTensor->dims;

        m_outputs.emplace_back(outputTensor->data.f, outputTensor->bytes, dims->data, dims->size);
    }
}

auto my::ModelLoader::isIndexValid(
    int idx, const char c
) const -> bool
{
    int size = 0;
    if (c == 'i')
    {
        size = m_inputs.size();
    }
    else if (c == 'o')
    {
        size = m_outputs.size();
    }
    else
    {
        return false;
    }

    if (idx < 0 || idx >= size)
    {
        std::cerr << "Index " << idx << " is out of range (" << size << ")." << '\n';
        return false;
    }
    return true;
}

auto my::ModelLoader::isAllInputsLoaded() const -> bool
{
    return (std::find(m_inputLoads.begin(), m_inputLoads.end(), false) == m_inputLoads.end());
}

void my::ModelLoader::inputChecker()
{
    if (!isAllInputsLoaded())
    {
        std::cerr << "Input ";
        for (int i = 0; i < m_inputLoads.size(); ++i)
        {
            if (!m_inputLoads[i])
            {
                std::cerr << i << " ";
            }
        }
        std::cerr << "haven't been loaded." << '\n';
        std::exit(1);
    }
    std::fill(m_inputLoads.begin(), m_inputLoads.end(), false);
}

auto my::ModelLoader::preprocessImage(
    const cv::Mat& in, int idx
) const -> cv::Mat
{
    auto out = convertToRGB(in);

    std::vector<int> inputShape = getInputShape(idx);
    int const H = inputShape[1];
    int const W = inputShape[2];

    cv::Size const wantedSize = cv::Size(W, H);
    cv::resize(out, out, wantedSize);

    /*
    Equivalent to (out - mean)/ std
    */
    out.convertTo(out, CV_32FC3, 1 / INPUT_NORM_STD, -INPUT_NORM_MEAN / INPUT_NORM_STD);
    return out;
}

auto my::ModelLoader::convertToRGB(
    const cv::Mat& in
) -> cv::Mat
{
    cv::Mat out;
    int const type = in.type();

    if (type == CV_8UC3)
    {
        cv::cvtColor(in, out, cv::COLOR_BGR2RGB);
    }
    else if (type == CV_8UC4)
    {
        cv::cvtColor(in, out, cv::COLOR_BGRA2RGB);
    }
    else
    {
        std::cerr << "Image of type " << type << " not supported" << '\n';
        std::exit(1);
    }
    return out;
}
