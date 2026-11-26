#include "Adapters/cpp/TestSupport/FakeProcessor.hpp"
#include "DataProcessor.hpp"
#include "ufeel/Model/Status.hpp"
#include "ufeel/Pipeline/LatestFrameCapture.hpp"

#include <gtest/gtest.h>

TEST(
    LatestFrameCaptureTest, CapturesFrames
)
{
    auto [backend, processor] = makeFakeProcessor();
    ufeel::LatestFrameCapture capture;

    EXPECT_EQ(capture.capture(processor), ufeel::Status::Ok);
    EXPECT_EQ(capture.capture(processor), ufeel::Status::Ok);
}

TEST(
    LatestFrameCaptureTest, ReportsUnavailableCameras
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->updateFrameStatus = DataProcessorStatus::FrameUnavailable;
    ufeel::LatestFrameCapture capture;

    EXPECT_EQ(capture.capture(processor), ufeel::Status::InvalidFrame);
}
