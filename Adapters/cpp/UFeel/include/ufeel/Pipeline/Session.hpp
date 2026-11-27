#ifndef UFEEL_PIPELINE_SESSION_HPP
#define UFEEL_PIPELINE_SESSION_HPP

#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/Frame.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"
#include "ufeel/Model/Status.hpp"
#include "ufeel/Native/Processor.hpp"
#include "ufeel/Pipeline/Clock.hpp"
#include "ufeel/Pipeline/EyeTrackingSmoother.hpp"
#include "ufeel/Pipeline/FeatureDemand.hpp"
#include "ufeel/Pipeline/FeatureScheduler.hpp"
#include "ufeel/Pipeline/LatestFrameCapture.hpp"
#include "ufeel/Pipeline/Snapshot.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace ufeel
{

struct SessionOptions
{
    FeatureIntervals intervals = FeatureIntervals();
    Clock::duration demandTimeout = std::chrono::seconds(2);

    // ! called on the session thread: they must not block or throw.
    std::function<void(Processor&)> onFrameCaptured = nullptr;
    std::function<void(const std::shared_ptr<const Snapshot>&)> onSnapshot = nullptr;
    std::function<void()> onMicrophonesChanged = nullptr;
};

class Session
{
public:

    explicit Session(Processor processor, SessionOptions options = {});
    ~Session();

    Session(const Session&) = delete;
    auto operator=(const Session&) -> Session& = delete;
    Session(Session&&) = delete;
    auto operator=(Session&&) -> Session& = delete;

    void start();

    [[nodiscard]] auto demand() -> FeatureDemand&;
    [[nodiscard]] auto scheduler() -> FeatureScheduler&;

    [[nodiscard]] auto latest() const -> std::shared_ptr<const Snapshot>;
    [[nodiscard]] auto supportedFeatures() const -> DetectionType;
    [[nodiscard]] auto isSpeechToTextEnabled() const -> bool;
    [[nodiscard]] auto selectedMicrophoneId() const -> std::string;

    template <typename Function>
    auto execute(Function&& function)
        -> std::future<std::invoke_result_t<std::decay_t<Function>&, Processor&>>;

    auto setSpeechToTextEnabled(bool enabled) -> std::future<void>;
    auto microphones() -> std::future<std::vector<Microphone>>;
    auto selectMicrophone(std::string id) -> std::future<bool>;

private:

    using Command = std::function<void(Processor&)>;

    void enqueue(Command command);

    void run(const std::stop_token& stop);
    void runCommands(const std::stop_token& stop);
    void waitForCommands(const std::stop_token& stop, Clock::duration timeout);
    void checkMicrophones();
    void step(const std::stop_token& stop);

    [[nodiscard]] auto captureFrame() -> bool;
    [[nodiscard]] auto process(DetectionType due) -> Status;
    [[nodiscard]] auto update(DetectionType features) -> Status;
    void readResults(DetectionType features);
    void resetInactive(DetectionType active);
    void markUnsupported(DetectionType feature);

    [[nodiscard]] auto frameMessage(bool needsFrames) const -> std::string;
    void publish(std::string message);

    Processor processor_;
    SessionOptions options_;
    FeatureDemand demand_;
    FeatureScheduler scheduler_;
    LatestFrameCapture capture_;
    EyeTrackingSmoother smoother_;
    Frame frame_;

    EmotionResult emotions_;
    EyeTrackingResult eyeTracking_;
    SpeechToTextResult speechToText_;
    HeartRateResult heartRate_;
    DetectionType lastActive_ = DetectionType::None;
    bool frameAvailable_ = false;
    std::uint64_t sequence_ = 0;
    std::optional<Clock::time_point> lastMicrophoneCheck_;

    mutable std::mutex latestMutex_;
    std::shared_ptr<const Snapshot> latest_ = std::make_shared<const Snapshot>();
    std::atomic<DetectionType> supported_ = allDetectionTypes;
    std::atomic<bool> speechEnabled_ = false;
    mutable std::mutex microphoneMutex_;
    std::string selectedMicrophoneId_;

    std::mutex commandsMutex_;
    std::condition_variable_any commandsAvailable_;
    std::deque<Command> commands_;

    std::jthread thread_;
};

template <typename Function>
auto Session::execute(
    Function&& function
) -> std::future<std::invoke_result_t<std::decay_t<Function>&, Processor&>>
{
    using Result = std::invoke_result_t<std::decay_t<Function>&, Processor&>;

    auto task =
        std::make_shared<std::packaged_task<Result(Processor&)>>(std::forward<Function>(function));
    auto future = task->get_future();
    enqueue([task = std::move(task)](Processor& processor) -> void { (*task)(processor); });
    return future;
}

} // namespace ufeel

#endif // UFEEL_PIPELINE_SESSION_HPP
