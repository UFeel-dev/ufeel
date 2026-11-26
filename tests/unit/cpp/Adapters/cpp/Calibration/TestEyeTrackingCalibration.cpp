#include "Adapters/cpp/TestSupport/Eventually.hpp"
#include "Adapters/cpp/TestSupport/FakeProcessor.hpp"
#include "Adapters/cpp/TestSupport/TemporaryDirectory.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "Wrapper/support/ProcessorBackendFake.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "ufeel/Calibration/CalibrationPresenter.hpp"
#include "ufeel/Calibration/CalibrationResult.hpp"
#include "ufeel/Calibration/EyeTrackingCalibration.hpp"
#include "ufeel/Configuration/CalibrationProfileStore.hpp"
#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Pipeline/Clock.hpp"
#include "ufeel/Pipeline/Session.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

constexpr ufeel::CalibrationTiming fastTiming{
    .steadyFace = ufeel::Clock::duration::zero(),
    .readinessPoll = ufeel::Clock::duration::zero(),
    .maxPerTarget = std::chrono::seconds(5),
    .progressPoll = ufeel::Clock::duration::zero(),
};

class RecordingPresenter final : public ufeel::CalibrationPresenter
{
public:

    bool visible = false;
    int hides = 0;
    std::vector<ufeel::CalibrationTarget> targets;
    std::string instruction;
    std::string status;

    void show() override
    {
        visible = true;
    }

    void hide() override
    {
        visible = false;
        ++hides;
    }

    void showTarget(
        float x, float y
    ) override
    {
        targets.push_back({.x = x, .y = y});
    }

    void showInstruction(
        std::string_view text
    ) override
    {
        instruction = text;
    }

    void showStatus(
        std::string_view text
    ) override
    {
        status = text;
    }
};

class EyeTrackingCalibrationTest : public ::testing::Test
{
protected:

    void SetUp() override
    {
        auto [fake, processor] = makeFakeProcessor();
        fake->frame = cv::Mat(4, 4, CV_8UC3, cv::Scalar::all(10));
        fake->eyeTracking.status = EyeTrackingStatus::Calibrating;
        fake->calibrationProgress.samplesForCurrentTarget = 5;
        backend_ = fake;
        session_ = std::make_unique<ufeel::Session>(std::move(processor));
    }

    [[nodiscard]] auto backend() const -> ProcessorBackendFake&
    {
        return *backend_;
    }

    [[nodiscard]] auto session() const -> ufeel::Session&
    {
        return *session_;
    }

    static auto runToEnd(
        ufeel::EyeTrackingCalibration& calibration
    ) -> ufeel::CalibrationResult
    {
        std::optional<ufeel::CalibrationResult> result;
        static_cast<void>(eventually([&calibration, &result] -> bool {
            result = calibration.update();
            return result.has_value();
        }));
        if (!result.has_value())
        {
            throw std::runtime_error("The calibration did not finish.");
        }
        return *result;
    }

    [[nodiscard]] auto eyeTrackingPinned() const -> bool
    {
        return ufeel::hasAny(session_->demand().active(), ufeel::DetectionType::EyeTracking);
    }

private:

    ProcessorBackendFake* backend_ = nullptr;
    std::unique_ptr<ufeel::Session> session_;
};

} // namespace

TEST_F(
    EyeTrackingCalibrationTest, CalibratesEveryTargetAndSavesTheProfile
)
{
    const TemporaryDirectory directory;
    const ufeel::CalibrationProfileStore profiles(directory.path() / "nested" / "profile.txt");
    RecordingPresenter presenter;
    ufeel::EyeTrackingCalibration calibration(session(), presenter, &profiles, fastTiming);
    session().start();

    calibration.start();
    EXPECT_TRUE(calibration.isRunning());
    EXPECT_TRUE(eyeTrackingPinned());
    const ufeel::CalibrationResult result = runToEnd(calibration);

    EXPECT_EQ(result.outcome, ufeel::CalibrationOutcome::Succeeded);
    EXPECT_TRUE(result.profileSaved);
    EXPECT_EQ(result.message, "Eye tracking calibrated and profile saved");
    EXPECT_FALSE(calibration.isRunning());
    EXPECT_FALSE(eyeTrackingPinned());
    EXPECT_FALSE(presenter.visible);
    EXPECT_EQ(presenter.hides, 1);
    ASSERT_EQ(presenter.targets.size(), ufeel::EyeTrackingCalibration::targets.size() + 1);
    EXPECT_FLOAT_EQ(presenter.targets.back().x, ufeel::EyeTrackingCalibration::targets.back().x);
    EXPECT_FLOAT_EQ(presenter.targets.back().y, ufeel::EyeTrackingCalibration::targets.back().y);
}

TEST_F(
    EyeTrackingCalibrationTest, ReportsDiagnosticsWhenTheFitFails
)
{
    backend().calibrationFinished = false;
    RecordingPresenter presenter;
    ufeel::EyeTrackingCalibration calibration(session(), presenter, nullptr, fastTiming);
    session().start();

    calibration.start();
    const ufeel::CalibrationResult result = runToEnd(calibration);

    EXPECT_EQ(result.outcome, ufeel::CalibrationOutcome::Failed);
    if (!result.diagnostics.has_value())
    {
        FAIL() << "a failed calibration keeps its diagnostics";
    }
    EXPECT_EQ(
        result.diagnostics->failureReason, ufeel::CalibrationFailureReason::InsufficientSamples
    );
    EXPECT_FALSE(presenter.visible);
}

TEST_F(
    EyeTrackingCalibrationTest, WaitsForASteadyFace
)
{
    backend().eyeTracking.status = EyeTrackingStatus::FaceUnavailable;
    RecordingPresenter presenter;
    ufeel::EyeTrackingCalibration calibration(session(), presenter, nullptr, fastTiming);
    session().start();

    calibration.start();

    EXPECT_TRUE(eventually([&calibration, &presenter] -> bool {
        static_cast<void>(calibration.update());
        return presenter.status == "Waiting for a valid face and eye landmarks";
    }));
    EXPECT_TRUE(presenter.visible);
    EXPECT_EQ(presenter.targets.size(), 1U);
}

TEST_F(
    EyeTrackingCalibrationTest, CancelsOnRequest
)
{
    backend().eyeTracking.status = EyeTrackingStatus::FaceUnavailable;
    RecordingPresenter presenter;
    ufeel::EyeTrackingCalibration calibration(session(), presenter, nullptr, fastTiming);
    session().start();
    calibration.start();
    ASSERT_TRUE(eventually([&calibration, &presenter] -> bool {
        static_cast<void>(calibration.update());
        return presenter.visible;
    }));

    calibration.cancel();
    const ufeel::CalibrationResult result = runToEnd(calibration);

    EXPECT_EQ(result.outcome, ufeel::CalibrationOutcome::Cancelled);
    EXPECT_FALSE(calibration.isRunning());
    EXPECT_FALSE(presenter.visible);
    EXPECT_FALSE(eyeTrackingPinned());
    EXPECT_EQ(calibration.update(), std::nullopt);
}

TEST_F(
    EyeTrackingCalibrationTest, RejectsASecondStart
)
{
    RecordingPresenter presenter;
    ufeel::EyeTrackingCalibration calibration(session(), presenter, nullptr, fastTiming);

    calibration.start();

    EXPECT_THROW(calibration.start(), std::logic_error);
}

TEST_F(
    EyeTrackingCalibrationTest, ReleasesTheSessionWhenDestroyed
)
{
    RecordingPresenter presenter;
    {
        ufeel::EyeTrackingCalibration calibration(session(), presenter, nullptr, fastTiming);
        calibration.start();
        EXPECT_TRUE(eyeTrackingPinned());
    }

    EXPECT_FALSE(eyeTrackingPinned());
}
