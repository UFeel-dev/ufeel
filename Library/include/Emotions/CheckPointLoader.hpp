#ifndef CHECKPOINTLOADER_HPP_
#define CHECKPOINTLOADER_HPP_

#include "EmotionModel.hpp"

#include <torch/torch.h>

auto loadCheckpoint(EmotionModel model, const std::string& path, torch::Device device = torch::kCPU)
    -> EmotionModel;

#endif /* !CHECKPOINTLOADER_HPP_ */
