#ifndef UFEELPROCESSOR_HPP
#define UFEELPROCESSOR_HPP

#include "DataProcessor.hpp"

#include <string>

struct UFeelProcessor
{
    DataProcessor processor;

    UFeelProcessor(const std::string& modelDirectory, const std::string& speechModelPath);
};

#endif // UFEELPROCESSOR_HPP
