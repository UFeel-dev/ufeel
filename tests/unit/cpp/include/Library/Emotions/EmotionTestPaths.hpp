#ifndef LIBRARY_EMOTIONS_EMOTIONTESTPATHS_HPP
#define LIBRARY_EMOTIONS_EMOTIONTESTPATHS_HPP

#include <filesystem>

#ifndef EMOTIONS_MODEL_DIR_PATH
#define EMOTIONS_MODEL_DIR_PATH ""
#endif // EMOTIONS_MODEL_DIR_PATH

inline auto emotionModelDirectory() -> std::filesystem::path
{
    return EMOTIONS_MODEL_DIR_PATH;
}

inline auto emotionFaceDetectorModelPath() -> std::filesystem::path
{
    return emotionModelDirectory() / "Emotions/face_detection_yunet_2023mar.onnx";
}

#endif // LIBRARY_EMOTIONS_EMOTIONTESTPATHS_HPP
