#ifndef UFEEL_NATIVE_PROCESSOR_HPP
#define UFEEL_NATIVE_PROCESSOR_HPP

#include "ufeel.h"
#include "ufeel/Configuration/PackageLayout.hpp"
#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/Frame.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"
#include "ufeel/Model/Status.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace ufeel
{

class Processor
{
public:

    explicit Processor(const PackageLayout& layout);
    Processor(
        const std::filesystem::path& modelDirectory, const std::filesystem::path& speechModelPath
    );

    [[nodiscard]] static auto adopt(UFeelProcessor* handle) -> Processor;

    [[nodiscard]] auto update(DetectionType features) -> Status;
    [[nodiscard]] auto updateFromFrame(DetectionType features, const Frame& frame) -> Status;
    [[nodiscard]] auto updateFrame() -> Status;

    [[nodiscard]] auto frameSize() -> std::optional<FrameSize>;
    [[nodiscard]] auto copyFrame(Frame& destination) -> bool;
    [[nodiscard]] auto
    copyFrameBgra(std::span<std::uint8_t> destination, int rowBytes, FrameSize size) -> bool;

    [[nodiscard]] auto emotions() -> EmotionResult;
    [[nodiscard]] auto eyeTracking() -> EyeTrackingResult;
    [[nodiscard]] auto speechToText() -> SpeechToTextResult;
    [[nodiscard]] auto heartRate() -> HeartRateResult;

    void setSpeechToTextEnabled(bool enabled);
    void setEyeTrackingHeadWeight(float weight);

    [[nodiscard]] auto microphones() -> std::vector<Microphone>;
    [[nodiscard]] auto selectMicrophone(const std::string& id) -> bool;
    [[nodiscard]] auto haveMicrophonesChanged() -> bool;

    void beginEyeTrackingCalibration();
    void setEyeTrackingCalibrationTarget(float screenX, float screenY);
    [[nodiscard]] auto finishEyeTrackingCalibration() -> bool;
    void cancelEyeTrackingCalibration();
    [[nodiscard]] auto eyeTrackingCalibrationProgress() -> CalibrationProgress;
    [[nodiscard]] auto eyeTrackingCalibrationDiagnostics() -> CalibrationDiagnostics;
    [[nodiscard]] auto saveEyeTrackingCalibrationProfile(const std::filesystem::path& path) -> bool;
    [[nodiscard]] auto loadEyeTrackingCalibrationProfile(const std::filesystem::path& path) -> bool;

private:

    struct Destroy
    {
        void operator()(UFeelProcessor* handle) const noexcept;
    };

    explicit Processor(UFeelProcessor* handle);

    [[nodiscard]] auto handle() const -> UFeelProcessor*;

    std::unique_ptr<UFeelProcessor, Destroy> handle_;
};

} // namespace ufeel

#endif // UFEEL_NATIVE_PROCESSOR_HPP
