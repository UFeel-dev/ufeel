#ifndef UFEEL_TEXT_TEXT_HPP
#define UFEEL_TEXT_TEXT_HPP

#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"

#include <span>
#include <string>
#include <string_view>

namespace ufeel::text
{

[[nodiscard]] auto emotions(const EmotionResult& result) -> std::string;
[[nodiscard]] auto dominantEmotion(const EmotionResult& result) -> std::string;
[[nodiscard]] auto speechToText(const SpeechToTextResult& result) -> std::string;
[[nodiscard]] auto defaultMicrophone(std::span<const Microphone> microphones) -> std::string;
[[nodiscard]] auto directions(const EyeTrackingResult& result) -> std::string;
[[nodiscard]] auto eyes(const EyeTrackingResult& result) -> std::string;
[[nodiscard]] auto eyeTracking(const EyeTrackingResult& result) -> std::string;
[[nodiscard]] auto heartRate(const HeartRateResult& result) -> std::string;
[[nodiscard]] auto calibrationFailure(const CalibrationDiagnostics& diagnostics) -> std::string;
[[nodiscard]] auto failureReason(CalibrationFailureReason reason) -> std::string_view;

} // namespace ufeel::text

#endif // UFEEL_TEXT_TEXT_HPP
