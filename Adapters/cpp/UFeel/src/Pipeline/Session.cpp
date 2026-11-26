#include "ufeel/Pipeline/Session.hpp"

#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"
#include "ufeel/Model/Status.hpp"
#include "ufeel/Native/Error.hpp"
#include "ufeel/Native/Processor.hpp"
#include "ufeel/Pipeline/Clock.hpp"
#include "ufeel/Pipeline/FeatureDemand.hpp"
#include "ufeel/Pipeline/FeatureScheduler.hpp"
#include "ufeel/Pipeline/Snapshot.hpp"

#include <chrono>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace ufeel
{

namespace
{

constexpr std::string_view frameUnavailableMessage = "Camera frame unavailable";
constexpr auto idleWait = std::chrono::milliseconds(20);
constexpr auto frameRetryWait = std::chrono::milliseconds(50);
constexpr auto errorRetryWait = std::chrono::milliseconds(500);
constexpr auto microphoneCheckInterval = std::chrono::seconds(1);

} // namespace

Session::Session(
    Processor processor, SessionOptions options
)
    : processor_(std::move(processor))
    , options_(std::move(options))
    , demand_(options_.demandTimeout)
    , scheduler_(options_.intervals)
{
}

Session::~Session() = default;

void Session::start()
{
    if (thread_.joinable())
    {
        throw std::logic_error("The UFeel session is already started.");
    }

    thread_ = std::jthread([this](const std::stop_token& stop) -> void { run(stop); });
}

auto Session::demand() -> FeatureDemand&
{
    return demand_;
}

auto Session::scheduler() -> FeatureScheduler&
{
    return scheduler_;
}

auto Session::latest() const -> std::shared_ptr<const Snapshot>
{
    const std::scoped_lock lock(latestMutex_);
    return latest_;
}

auto Session::supportedFeatures() const -> DetectionType
{
    return supported_.load();
}

auto Session::isSpeechToTextEnabled() const -> bool
{
    return speechEnabled_.load();
}

auto Session::selectedMicrophoneId() const -> std::string
{
    const std::scoped_lock lock(microphoneMutex_);
    return selectedMicrophoneId_;
}

auto Session::setSpeechToTextEnabled(
    bool enabled
) -> std::future<void>
{
    return execute([this, enabled](Processor& processor) -> void {
        processor.setSpeechToTextEnabled(enabled);
        if (speechEnabled_.exchange(enabled) == enabled)
        {
            return;
        }

        if (enabled)
        {
            demand_.pin(DetectionType::SpeechToText);
            return;
        }

        demand_.unpin(DetectionType::SpeechToText);
        speechToText_ = {};
        publish(latest()->message);
    });
}

auto Session::microphones() -> std::future<std::vector<Microphone>>
{
    return execute([](Processor& processor) -> std::vector<Microphone> {
        return processor.microphones();
    });
}

auto Session::selectMicrophone(
    std::string id
) -> std::future<bool>
{
    return execute([this, id = std::move(id)](Processor& processor) -> bool {
        if (!processor.selectMicrophone(id))
        {
            return false;
        }

        const std::scoped_lock lock(microphoneMutex_);
        selectedMicrophoneId_ = id;
        return true;
    });
}

void Session::enqueue(
    Command command
)
{
    {
        const std::scoped_lock lock(commandsMutex_);
        commands_.push_back(std::move(command));
    }
    commandsAvailable_.notify_one();
}

void Session::run(
    const std::stop_token& stop
)
{
    while (!stop.stop_requested())
    {
        try
        {
            runCommands(stop);
            checkMicrophones();
            step(stop);
        }
        catch (const std::exception& error)
        {
            publish(error.what());
            waitForCommands(stop, errorRetryWait);
        }
    }
}

void Session::runCommands(
    const std::stop_token& stop
)
{
    while (!stop.stop_requested())
    {
        Command command;
        {
            const std::scoped_lock lock(commandsMutex_);
            if (commands_.empty())
            {
                return;
            }
            command = std::move(commands_.front());
            commands_.pop_front();
        }
        command(processor_);
    }
}

void Session::waitForCommands(
    const std::stop_token& stop, Clock::duration timeout
)
{
    std::unique_lock lock(commandsMutex_);
    commandsAvailable_.wait_for(lock, stop, timeout, [this]() -> bool {
        return !commands_.empty();
    });
}

void Session::checkMicrophones()
{
    if (!options_.onMicrophonesChanged)
    {
        return;
    }

    const auto now = Clock::now();
    if (lastMicrophoneCheck_ && now - *lastMicrophoneCheck_ < microphoneCheckInterval)
    {
        return;
    }

    lastMicrophoneCheck_ = now;
    if (processor_.haveMicrophonesChanged())
    {
        options_.onMicrophonesChanged();
    }
}

void Session::step(
    const std::stop_token& stop
)
{
    const DetectionType active = demand_.active(Clock::now()) & supportedFeatures();
    const bool activeChanged = active != lastActive_;
    if (activeChanged)
    {
        resetInactive(active);
        lastActive_ = active;
    }

    const bool needsFrames =
        hasAny(active, imageDetectionTypes) || options_.onFrameCaptured != nullptr;
    if (!needsFrames)
    {
        capture_.reset();
    }

    const bool wasAvailable = frameAvailable_;
    frameAvailable_ = needsFrames && captureFrame();

    const DetectionType due =
        scheduler_.takeDue(Clock::now(), frameAvailable_ ? active : active & ~imageDetectionTypes);
    if (due != DetectionType::None)
    {
        const Status status = process(due);
        publish(
            status == Status::Ok || status == Status::UnsupportedFeature
                ? frameMessage(needsFrames)
                : "Detection failed (" + std::string(toString(status)) +
                      "), retrying on the next frame"
        );
        return;
    }

    if (needsFrames && !frameAvailable_)
    {
        if (wasAvailable || activeChanged || sequence_ == 0)
        {
            publish(std::string(frameUnavailableMessage));
        }
        waitForCommands(stop, frameRetryWait);
        return;
    }

    if (activeChanged)
    {
        publish(frameMessage(needsFrames));
    }

    if (!needsFrames)
    {
        waitForCommands(stop, idleWait);
    }
}

auto Session::captureFrame() -> bool
{
    const Status status = capture_.capture(processor_);
    if (status == Status::Ok)
    {
        if (options_.onFrameCaptured)
        {
            options_.onFrameCaptured(processor_);
        }
        return true;
    }

    if (status != Status::InvalidFrame)
    {
        throw Error(status, "capture a camera frame");
    }

    emotions_ = {.status = EmotionStatus::FrameUnavailable, .scores = {}};
    eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
    smoother_.reset();
    return false;
}

auto Session::process(
    DetectionType due
) -> Status
{
    const Status status = update(due);
    if (status == Status::Ok)
    {
        readResults(due);
        return status;
    }

    if (status != Status::UnsupportedFeature)
    {
        return status;
    }

    Status combined = Status::Ok;
    for (const DetectionType feature : singleDetectionTypes)
    {
        if (!hasAny(due, feature))
        {
            continue;
        }

        const Status single = update(feature);
        if (single == Status::Ok)
        {
            readResults(feature);
        }
        else if (single == Status::UnsupportedFeature)
        {
            markUnsupported(feature);
        }
        else
        {
            combined = single;
        }
    }

    return combined;
}

auto Session::update(
    DetectionType features
) -> Status
{
    if (!hasAny(features, imageDetectionTypes))
    {
        return processor_.update(features);
    }

    return processor_.copyFrame(frame_) ? processor_.updateFromFrame(features, frame_)
                                        : Status::InvalidFrame;
}

void Session::readResults(
    DetectionType features
)
{
    if (hasAny(features, DetectionType::Emotions))
    {
        emotions_ = processor_.emotions();
    }

    if (hasAny(features, DetectionType::EyeTracking))
    {
        eyeTracking_ = smoother_.smooth(processor_.eyeTracking());
    }

    if (hasAny(features, DetectionType::SpeechToText))
    {
        speechToText_ = speechEnabled_.load() ? processor_.speechToText() : SpeechToTextResult{};
    }

    if (hasAny(features, DetectionType::HeartRate))
    {
        heartRate_ = processor_.heartRate();
    }
}

void Session::resetInactive(
    DetectionType active
)
{
    if (!hasAny(active, DetectionType::Emotions))
    {
        emotions_ = {};
    }

    if (!hasAny(active, DetectionType::EyeTracking))
    {
        eyeTracking_ = {};
        smoother_.reset();
    }

    if (!hasAny(active, DetectionType::HeartRate))
    {
        heartRate_ = {};
    }
}

void Session::markUnsupported(
    DetectionType feature
)
{
    supported_.store(supported_.load() & ~feature);

    switch (feature)
    {
    case DetectionType::Emotions:
        emotions_ = {.status = EmotionStatus::UnsupportedFeature, .scores = {}};
        break;
    case DetectionType::EyeTracking:
        eyeTracking_ = {.status = EyeTrackingStatus::Disabled};
        break;
    case DetectionType::SpeechToText:
        speechToText_ = {.status = SpeechToTextStatus::Disabled, .text = {}};
        break;
    case DetectionType::HeartRate:
        heartRate_ = {.status = HeartRateStatus::NoData};
        break;
    case DetectionType::None:
        break;
    }
}

auto Session::frameMessage(
    bool needsFrames
) const -> std::string
{
    return needsFrames && !frameAvailable_ ? std::string(frameUnavailableMessage) : std::string();
}

void Session::publish(
    std::string message
)
{
    auto snapshot = std::make_shared<const Snapshot>(Snapshot{
        .sequence = ++sequence_,
        .frameAvailable = frameAvailable_,
        .message = std::move(message),
        .emotions = emotions_,
        .eyeTracking = eyeTracking_,
        .speechToText = speechToText_,
        .heartRate = heartRate_,
    });

    {
        const std::scoped_lock lock(latestMutex_);
        latest_ = snapshot;
    }

    if (options_.onSnapshot)
    {
        options_.onSnapshot(snapshot);
    }
}

} // namespace ufeel
