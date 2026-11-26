#include "SpeechToText/CoreAudioDevices.hpp"

#include <Block.h>
#include <CoreAudio/AudioHardware.h>
#include <array>
#include <atomic>
#include <dispatch/dispatch.h>
#include <functional>
#include <memory>

namespace
{

constexpr std::array kWatchedProperties{
    AudioObjectPropertyAddress{
        .mSelector = kAudioHardwarePropertyDevices,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMain,
    },
    AudioObjectPropertyAddress{
        .mSelector = kAudioHardwarePropertyDefaultInputDevice,
        .mScope = kAudioObjectPropertyScopeGlobal,
        .mElement = kAudioObjectPropertyElementMain,
    },
};

/** Listens on a private queue, so notifications arrive without a run loop on the main thread. */
class CoreAudioDeviceMonitor final
{
public:

    CoreAudioDeviceMonitor()
        : queue_(dispatch_queue_create("UFeel.CoreAudioDevices", DISPATCH_QUEUE_SERIAL))
    {
        // The block owns a copy of the flag, so a notification already queued when the monitor is
        // destroyed never writes to freed memory.
        const std::shared_ptr<std::atomic<bool>> changed = changed_;

        listener_ =
            Block_copy(^(UInt32 /*count*/, const AudioObjectPropertyAddress* /*addresses*/) {
              changed->store(true, std::memory_order_relaxed);
            });

        for (const auto& property : kWatchedProperties)
        {
            AudioObjectAddPropertyListenerBlock(
                kAudioObjectSystemObject, &property, queue_, listener_
            );
        }
    }

    ~CoreAudioDeviceMonitor()
    {
        for (const auto& property : kWatchedProperties)
        {
            AudioObjectRemovePropertyListenerBlock(
                kAudioObjectSystemObject, &property, queue_, listener_
            );
        }

        Block_release(listener_);
        dispatch_release(queue_);
    }

    CoreAudioDeviceMonitor(const CoreAudioDeviceMonitor&) = delete;
    auto operator=(const CoreAudioDeviceMonitor&) -> CoreAudioDeviceMonitor& = delete;

    CoreAudioDeviceMonitor(CoreAudioDeviceMonitor&&) = delete;
    auto operator=(CoreAudioDeviceMonitor&&) -> CoreAudioDeviceMonitor& = delete;

    [[nodiscard]] auto takeChanged() -> bool
    {
        return changed_->exchange(false, std::memory_order_relaxed);
    }

private:

    std::shared_ptr<std::atomic<bool>> changed_ = std::make_shared<std::atomic<bool>>(false);
    dispatch_queue_t queue_;
    AudioObjectPropertyListenerBlock listener_ = nullptr;
};

} // namespace

auto watchCoreAudioDevices() -> std::function<bool()>
{
    auto monitor = std::make_shared<CoreAudioDeviceMonitor>();
    return [monitor] -> bool { return monitor->takeChanged(); };
}
