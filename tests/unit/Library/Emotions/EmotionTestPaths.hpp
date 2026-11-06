#ifndef TESTS_UNIT_EMOTIONS_EMOTIONTESTPATHS_HPP
#define TESTS_UNIT_EMOTIONS_EMOTIONTESTPATHS_HPP

#include <filesystem>

#ifndef EMOTIONS_MODEL_DIR_PATH
#define EMOTIONS_MODEL_DIR_PATH "" // TODO: find a cleaner way or shutdown any warning on this line
#endif

inline auto emotionModelDirectory() -> std::filesystem::path
{
    return EMOTIONS_MODEL_DIR_PATH;
}

inline auto emotionFaceDetectorModelPath() -> std::filesystem::path
{
    return emotionModelDirectory() / "Emotions/face_detection_yunet_2023mar.onnx";
}

#endif // TESTS_UNIT_EMOTIONS_EMOTIONTESTPATHS_HPP
