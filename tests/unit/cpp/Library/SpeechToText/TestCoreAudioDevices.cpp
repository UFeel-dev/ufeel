#include "SpeechToText/CoreAudioDevices.hpp"

#include <CoreAudio/AudioHardware.h>
#include <CoreFoundation/CoreFoundation.h>
#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <unistd.h>

namespace
{

constexpr auto kNotificationTimeout = std::chrono::seconds(5);
constexpr auto kNotificationSettleTime = std::chrono::milliseconds(200);

/** An aggregate device that only this process can see, so the test leaves the system untouched. */
class PrivateAggregateDevice final
{
public:

    PrivateAggregateDevice()
    {
        // ctest runs the tests in parallel processes, and two devices cannot share a UID.
        static std::atomic<int> created{0};
        const std::string uid = "UFeel.CoreAudioDevicesTest." + std::to_string(::getpid()) + "." +
                                std::to_string(created++);
        CFStringRef uidValue =
            CFStringCreateWithCString(nullptr, uid.c_str(), kCFStringEncodingUTF8);
        int isPrivate = 1;
        CFNumberRef privateValue = CFNumberCreate(nullptr, kCFNumberIntType, &isPrivate);
        std::array<const void*, 3> keys{
            CFSTR(kAudioAggregateDeviceUIDKey),
            CFSTR(kAudioAggregateDeviceNameKey),
            CFSTR(kAudioAggregateDeviceIsPrivateKey),
        };
        std::array<const void*, 3> values{
            uidValue,
            CFSTR("UFeel CoreAudioDevicesTest"),
            privateValue,
        };
        CFDictionaryRef description = CFDictionaryCreate(
            nullptr,
            keys.data(),
            values.data(),
            static_cast<CFIndex>(keys.size()),
            &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks
        );

        created_ = AudioHardwareCreateAggregateDevice(description, &device_) == noErr;

        CFRelease(description);
        CFRelease(privateValue);
        CFRelease(uidValue);
    }

    ~PrivateAggregateDevice()
    {
        if (created_)
        {
            AudioHardwareDestroyAggregateDevice(device_);
        }
    }

    PrivateAggregateDevice(const PrivateAggregateDevice&) = delete;
    auto operator=(const PrivateAggregateDevice&) -> PrivateAggregateDevice& = delete;

    PrivateAggregateDevice(PrivateAggregateDevice&&) = delete;
    auto operator=(PrivateAggregateDevice&&) -> PrivateAggregateDevice& = delete;

    [[nodiscard]] auto created() const -> bool
    {
        return created_;
    }

private:

    AudioObjectID device_ = kAudioObjectUnknown;
    bool created_ = false;
};

auto waitForChange(
    const std::function<bool()>& changed
) -> bool
{
    const auto deadline = std::chrono::steady_clock::now() + kNotificationTimeout;

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (changed())
        {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return false;
}

} // namespace

TEST(
    CoreAudioDevicesTest, ReportsNoChangeBeforeCoreAudioNotifiesOne
)
{
    const auto changed = watchCoreAudioDevices();

    ASSERT_TRUE(changed);
    EXPECT_FALSE(changed());
    EXPECT_FALSE(changed());
}

TEST(
    CoreAudioDevicesTest, ReportsAddedAndRemovedDevices
)
{
    const auto changed = watchCoreAudioDevices();
    {
        const PrivateAggregateDevice device;
        ASSERT_TRUE(device.created());

        EXPECT_TRUE(waitForChange(changed));

        // Adding a device can send several notifications: clear them before removing it.
        std::this_thread::sleep_for(kNotificationSettleTime);
        static_cast<void>(changed());
    }

    EXPECT_TRUE(waitForChange(changed));
}

TEST(
    CoreAudioDevicesTest, StopsWatchingOnceTheLastCopyIsDestroyed
)
{
    auto changed = watchCoreAudioDevices();
    auto copy = changed;

    changed = nullptr;
    EXPECT_FALSE(copy());

    EXPECT_NO_THROW(copy = nullptr);
    const PrivateAggregateDevice device;
    EXPECT_TRUE(device.created());
}
