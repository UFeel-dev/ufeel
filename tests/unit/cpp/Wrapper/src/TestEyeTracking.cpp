#include "ufeel.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <limits>

namespace
{

auto callbackStatus() -> int32_t&
{
    static int32_t status = -1;
    return status;
}

void recordEyeTrackingResult(
    UFeelEyeTrackingResult* result
)
{
    callbackStatus() = result->status;
}

} // namespace

TEST(
    EyeTrackingApiTest, NullProcessorOperationsReturnInvalidProcessor
)
{
    EXPECT_EQ(ufeel_update_eye_tracking(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(
        ufeel_update_eye_tracking_from_frame(nullptr, nullptr), UFEEL_STATUS_INVALID_PROCESSOR
    );
    EXPECT_EQ(ufeel_begin_eye_tracking_calibration(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(
        ufeel_set_eye_tracking_calibration_target(nullptr, 0.5F, 0.5F),
        UFEEL_STATUS_INVALID_PROCESSOR
    );
    EXPECT_EQ(ufeel_finish_eye_tracking_calibration(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_cancel_eye_tracking_calibration(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_get_eye_tracking_result(nullptr, nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_progress(nullptr, nullptr),
        UFEEL_STATUS_INVALID_PROCESSOR
    );
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_diagnostics(nullptr, nullptr),
        UFEEL_STATUS_INVALID_PROCESSOR
    );
    EXPECT_EQ(
        ufeel_save_eye_tracking_calibration_profile(nullptr, nullptr),
        UFEEL_STATUS_INVALID_PROCESSOR
    );
    EXPECT_EQ(
        ufeel_load_eye_tracking_calibration_profile(nullptr, nullptr),
        UFEEL_STATUS_INVALID_PROCESSOR
    );
}

TEST(
    EyeTrackingApiTest, ResultGetterRejectsNullOutput
)
{
    auto* processor = reinterpret_cast<UFeelProcessor*>(1);

    EXPECT_EQ(ufeel_get_eye_tracking_result(processor, nullptr), UFEEL_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_progress(processor, nullptr),
        UFEEL_STATUS_INVALID_ARGUMENT
    );
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_diagnostics(processor, nullptr),
        UFEEL_STATUS_INVALID_ARGUMENT
    );
}

TEST(
    EyeTrackingApiTest, RejectsNonFiniteTargetsAndEmptyProfilePaths
)
{
    auto* processor = reinterpret_cast<UFeelProcessor*>(1);

    EXPECT_EQ(
        ufeel_set_eye_tracking_calibration_target(
            processor, std::numeric_limits<float>::quiet_NaN(), 0.5F
        ),
        UFEEL_STATUS_INVALID_ARGUMENT
    );
    EXPECT_EQ(
        ufeel_set_eye_tracking_calibration_target(
            processor, 0.5F, std::numeric_limits<float>::infinity()
        ),
        UFEEL_STATUS_INVALID_ARGUMENT
    );
    EXPECT_EQ(
        ufeel_save_eye_tracking_calibration_profile(processor, nullptr),
        UFEEL_STATUS_INVALID_ARGUMENT
    );
    EXPECT_EQ(
        ufeel_save_eye_tracking_calibration_profile(processor, ""), UFEEL_STATUS_INVALID_ARGUMENT
    );
    EXPECT_EQ(
        ufeel_load_eye_tracking_calibration_profile(processor, nullptr),
        UFEEL_STATUS_INVALID_ARGUMENT
    );
    EXPECT_EQ(
        ufeel_load_eye_tracking_calibration_profile(processor, ""), UFEEL_STATUS_INVALID_ARGUMENT
    );
}

TEST(
    EyeTrackingApiTest, FrameCallbackReportsInvalidProcessor
)
{
    callbackStatus() = -1;

    ufeel_get_eye_tracking_result_from_frame(nullptr, nullptr, recordEyeTrackingResult);

    EXPECT_EQ(callbackStatus(), UFEEL_GAZE_STATUS_INVALID_PROCESSOR);
}

TEST(
    EyeTrackingApiTest, ResultAllocationAndReleaseAreNullSafe
)
{
    auto* result = ufeel_create_eye_tracking_result();

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->status, 0);
    ufeel_free_eye_tracking_result(result);
    EXPECT_NO_THROW(ufeel_free_eye_tracking_result(nullptr));
    EXPECT_NO_THROW(ufeel_get_eye_tracking_result_from_frame(nullptr, nullptr, nullptr));
}
