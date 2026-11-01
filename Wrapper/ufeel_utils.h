// NOLINTBEGIN

#ifndef UFEEL_UTILS_H_
#define UFEEL_UTILS_H_

#include "DataProcessor.hpp"
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <exception>
#include <iostream>
#include <string>
#include <utility>

struct UFeelProcessor
{
    DataProcessor processor;

    UFeelProcessor(
        const std::string& modelDirectory, const std::string& speechModelPath
    )
        : processor(modelDirectory, speechModelPath)
    {
    }
};

template <typename Result, typename Function>
auto guarded(
    const char* operation, Result fallback, Function&& function
) noexcept -> Result
{
    try
    {
        return std::forward<Function>(function)();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "UFeel: " << operation << " failed: " << exception.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "UFeel: " << operation << " failed with an unknown exception\n";
    }

    return fallback;
}

template <typename Function>
void guardedVoid(
    const char* operation, Function&& function
) noexcept
{
    try
    {
        std::forward<Function>(function)();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "UFeel: " << operation << " failed: " << exception.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "UFeel: " << operation << " failed with an unknown exception\n";
    }
}

auto duplicateUtf8(const std::string& value) -> char*;
auto convertUFeelFrameToMat(const UFeelFrame* frame) -> cv::Mat;

#endif /* !UFEEL_UTILS_H_ */
