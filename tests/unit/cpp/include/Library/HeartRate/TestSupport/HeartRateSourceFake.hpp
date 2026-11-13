#ifndef LIBRARY_HEARTRATE_TESTSUPPORT_HEARTRATESOURCEFAKE_HPP
#define LIBRARY_HEARTRATE_TESTSUPPORT_HEARTRATESOURCEFAKE_HPP

#include "HeartRate/HeartRateSource.hpp"

class HeartRateSourceFake final : public HeartRateSource
{
public:

    explicit HeartRateSourceFake(
        HeartRateResult result
    )
        : result_(result)
    {
    }

    [[nodiscard]] auto read() const -> HeartRateResult override
    {
        return result_;
    }

private:

    HeartRateResult result_;
};

#endif // LIBRARY_HEARTRATE_TESTSUPPORT_HEARTRATESOURCEFAKE_HPP
