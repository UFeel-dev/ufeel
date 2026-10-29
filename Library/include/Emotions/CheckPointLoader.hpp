#ifndef EMOTIONS_CHECKPOINTLOADER_HPP
#define EMOTIONS_CHECKPOINTLOADER_HPP

#include "EmotionModel.hpp"

#include <filesystem>

void loadCheckpoint(EmotionModel& model, const std::filesystem::path& path);

#endif // EMOTIONS_CHECKPOINTLOADER_HPP
