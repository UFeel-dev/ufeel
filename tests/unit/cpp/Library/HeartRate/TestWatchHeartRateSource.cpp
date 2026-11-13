#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "HeartRate/WatchHeartRateSource.hpp"
#include "Library/HeartRate/TestSupport/HeartRateTestHelpers.hpp"

#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{

class ScopedNoFileLimit
{
public:

    ScopedNoFileLimit()
    {
        if (::getrlimit(RLIMIT_NOFILE, &original_) != 0)
        {
            return;
        }

        rlimit restricted = original_;
        restricted.rlim_cur = 0;
        active_ = ::setrlimit(RLIMIT_NOFILE, &restricted) == 0;
    }

    ScopedNoFileLimit(const ScopedNoFileLimit&) = default;
    ScopedNoFileLimit(ScopedNoFileLimit&&) = delete;
    auto operator=(const ScopedNoFileLimit&) -> ScopedNoFileLimit& = default;
    auto operator=(ScopedNoFileLimit&&) -> ScopedNoFileLimit& = delete;
    ~ScopedNoFileLimit()
    {
        if (active_)
        {
            static_cast<void>(::setrlimit(RLIMIT_NOFILE, &original_));
        }
    }

    [[nodiscard]] auto active() const -> bool
    {
        return active_;
    }

    auto restore() -> bool
    {
        if (!active_)
        {
            return false;
        }

        if (::setrlimit(RLIMIT_NOFILE, &original_) != 0)
        {
            return false;
        }

        active_ = false;
        return true;
    }

private:

    rlimit original_{};
    bool active_{false};
};

} // namespace

TEST(
    WatchHeartRateSourceTest, ReturnsNoDataUntilAValidPacketArrives
)
{
    const WatchHeartRateSource source(0);
    const std::uint16_t port = source.port();
    ASSERT_NE(port, 0U);

    EXPECT_EQ(source.read().status, HeartRateStatus::NoData);
    ASSERT_TRUE(sendHeartRatePacket(port, "72", 2));

    const auto result = waitForHeartRate(source, HeartRateStatus::Valid, 72);

    EXPECT_EQ(result.bpm, 72U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    WatchHeartRateSourceTest, AcceptsMinimumAndMaximumBpm
)
{
    const WatchHeartRateSource source(0);
    const std::uint16_t port = source.port();
    ASSERT_NE(port, 0U);

    ASSERT_TRUE(sendHeartRatePacket(port, "20", 2));
    EXPECT_EQ(waitForHeartRate(source, HeartRateStatus::Valid, 20).bpm, 20U);

    ASSERT_TRUE(sendHeartRatePacket(port, "250", 3));
    EXPECT_EQ(waitForHeartRate(source, HeartRateStatus::Valid, 250).bpm, 250U);
}

TEST(
    WatchHeartRateSourceTest, IgnoresMalformedAndOutOfRangePackets
)
{
    const WatchHeartRateSource source(0);
    const std::uint16_t port = source.port();
    ASSERT_NE(port, 0U);

    ASSERT_TRUE(sendHeartRatePacket(port, "", 0));
    ASSERT_TRUE(sendHeartRatePacket(port, "not-a-number", 12));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_EQ(source.read().status, HeartRateStatus::NoData);

    ASSERT_TRUE(sendHeartRatePacket(port, "19", 2));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_EQ(source.read().status, HeartRateStatus::NoData);

    ASSERT_TRUE(sendHeartRatePacket(port, "251", 3));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_EQ(source.read().status, HeartRateStatus::NoData);

    ASSERT_TRUE(sendHeartRatePacket(port, "99999999999999999999", 20));
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_EQ(source.read().status, HeartRateStatus::NoData);

    ASSERT_TRUE(sendHeartRatePacket(port, "72", 2));
    EXPECT_EQ(waitForHeartRate(source, HeartRateStatus::Valid, 72).bpm, 72U);
}

TEST(
    WatchHeartRateSourceTest, RemainsAvailableWhenPortCannotBeBound
)
{
    const int reservedSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(reservedSocket, 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = 0;
    socklen_t addressLength = sizeof(address);
    ASSERT_EQ(::bind(reservedSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
    ASSERT_EQ(
        ::getsockname(reservedSocket, reinterpret_cast<sockaddr*>(&address), &addressLength), 0
    );

    const WatchHeartRateSource source(ntohs(address.sin_port));

    EXPECT_EQ(source.read().bpm, 0U);
    EXPECT_EQ(source.read().status, HeartRateStatus::NoData);
    static_cast<void>(::close(reservedSocket));
}

TEST(
    WatchHeartRateSourceTest, ReturnsNoDataWhenSocketCannotBeCreated
)
{
    ScopedNoFileLimit fileLimit;
    ASSERT_TRUE(fileLimit.active());

    const WatchHeartRateSource source(0);

    ASSERT_TRUE(fileLimit.restore());
    EXPECT_EQ(source.read().bpm, 0U);
    EXPECT_EQ(source.read().status, HeartRateStatus::NoData);
}
