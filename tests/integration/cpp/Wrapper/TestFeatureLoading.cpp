#include "ufeel.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <memory>

#ifndef UFEEL_TEST_WRAPPER_PATH
#define UFEEL_TEST_WRAPPER_PATH ""
#endif // UFEEL_TEST_WRAPPER_PATH
#ifndef UFEEL_TEST_MODEL_DIR
#define UFEEL_TEST_MODEL_DIR ""
#endif // UFEEL_TEST_MODEL_DIR
#ifndef UFEEL_TEST_SPEECH_MODEL_DIR
#define UFEEL_TEST_SPEECH_MODEL_DIR ""
#endif // UFEEL_TEST_SPEECH_MODEL_DIR
#ifndef UFEEL_ENABLE_EMOTIONS
#define UFEEL_ENABLE_EMOTIONS 0
#endif // UFEEL_ENABLE_EMOTIONS
#ifndef UFEEL_ENABLE_EYE_TRACKING
#define UFEEL_ENABLE_EYE_TRACKING 0
#endif // UFEEL_ENABLE_EYE_TRACKING
#ifndef UFEEL_ENABLE_SPEECH_TO_TEXT
#define UFEEL_ENABLE_SPEECH_TO_TEXT 0
#endif // UFEEL_ENABLE_SPEECH_TO_TEXT
#ifndef UFEEL_ENABLE_HEART_RATE
#define UFEEL_ENABLE_HEART_RATE 0
#endif // UFEEL_ENABLE_HEART_RATE

namespace
{

constexpr std::array kExportedFunctions{
    "ufeel_begin_eye_tracking_calibration",
    "ufeel_cancel_eye_tracking_calibration",
    "ufeel_copy_frame",
    "ufeel_copy_frame_bgra",
    "ufeel_create",
    "ufeel_create_emotions_result",
    "ufeel_create_eye_tracking_result",
    "ufeel_create_heart_rate_result",
    "ufeel_create_speech_to_text_result",
    "ufeel_destroy",
    "ufeel_finish_eye_tracking_calibration",
    "ufeel_free_emotions_result",
    "ufeel_free_eye_tracking_result",
    "ufeel_free_frame",
    "ufeel_free_heart_rate_result",
    "ufeel_free_speech_to_text_result",
    "ufeel_get_emotions_result",
    "ufeel_get_emotions_result_from_frame",
    "ufeel_get_eye_tracking_calibration_diagnostics",
    "ufeel_get_eye_tracking_calibration_progress",
    "ufeel_get_eye_tracking_result",
    "ufeel_get_eye_tracking_result_from_frame",
    "ufeel_get_frame",
    "ufeel_get_frame_info",
    "ufeel_get_heart_rate_result",
    "ufeel_get_speech_to_text_result",
    "ufeel_load_eye_tracking_calibration_profile",
    "ufeel_save_eye_tracking_calibration_profile",
    "ufeel_set_eye_tracking_calibration_target",
    "ufeel_set_frame",
    "ufeel_set_speech_to_text_enabled",
    "ufeel_update",
    "ufeel_update_emotions",
    "ufeel_update_emotions_from_frame",
    "ufeel_update_eye_tracking",
    "ufeel_update_eye_tracking_from_frame",
    "ufeel_update_frame",
    "ufeel_update_from_frame",
    "ufeel_update_heart_rate",
    "ufeel_update_speech_to_text",
};

struct Feature
{
    const char* name;
    std::int32_t flag;
    bool enabled;
};

constexpr std::array<Feature, 4> kFeatures{{
    {.name = "emotions", .flag = 1 << 0, .enabled = UFEEL_ENABLE_EMOTIONS != 0},
    {.name = "eye tracking", .flag = 1 << 1, .enabled = UFEEL_ENABLE_EYE_TRACKING != 0},
    {.name = "speech to text", .flag = 1 << 2, .enabled = UFEEL_ENABLE_SPEECH_TO_TEXT != 0},
    {.name = "heart rate", .flag = 1 << 3, .enabled = UFEEL_ENABLE_HEART_RATE != 0},
},};

struct LibraryCloser
{
    void operator()(
        void* handle
    ) const
    {
        dlclose(handle);
    }
};

using Library = std::unique_ptr<void, LibraryCloser>;

auto openLibrary() -> Library
{
    return Library(dlopen(UFEEL_TEST_WRAPPER_PATH, RTLD_NOW | RTLD_LOCAL));
}

auto loadError() -> const char*
{
    return dlerror(); // NOLINT(concurrency-mt-unsafe): tests load the library on one thread.
}

template <typename Function>
auto resolve(
    const Library& library, const char* name
) -> Function
{
    return reinterpret_cast<Function>(dlsym(library.get(), name));
}

} // namespace

TEST(
    FeatureLoadingTest, ExportsEveryPublicFunction
)
{
    const auto library = openLibrary();
    ASSERT_NE(library, nullptr) << loadError();

    for (const auto* name : kExportedFunctions)
    {
        EXPECT_NE(dlsym(library.get(), name), nullptr) << name;
    }
}

TEST(
    FeatureLoadingTest, ReportsOnlyDisabledFeaturesAsUnsupported
)
{
    const auto library = openLibrary();
    ASSERT_NE(library, nullptr) << loadError();
    const auto create = resolve<decltype(&ufeel_create)>(library, "ufeel_create");
    const auto destroy = resolve<decltype(&ufeel_destroy)>(library, "ufeel_destroy");
    const auto updateFromFrame =
        resolve<decltype(&ufeel_update_from_frame)>(library, "ufeel_update_from_frame");
    ASSERT_NE(create, nullptr);
    ASSERT_NE(destroy, nullptr);
    ASSERT_NE(updateFromFrame, nullptr);

    const std::unique_ptr<UFeelProcessor, decltype(destroy)> processor(
        create(UFEEL_TEST_MODEL_DIR, UFEEL_TEST_SPEECH_MODEL_DIR), destroy
    );
    ASSERT_NE(processor, nullptr);

    constexpr int size = 64;
    constexpr int channels = 3;
    std::array<std::uint8_t, static_cast<std::size_t>(size * size * channels)> pixels{};
    const UFeelFrame frame{
        .data = pixels.data(),
        .width = size,
        .height = size,
        .channels = channels,
        .stride = size * channels,
        .dataSize = pixels.size(),
    };

    for (const auto& feature : kFeatures)
    {
        EXPECT_EQ(
            updateFromFrame(processor.get(), feature.flag, &frame),
            feature.enabled ? UFEEL_STATUS_OK : UFEEL_STATUS_UNSUPPORTED_FEATURE
        ) << feature.name;
    }
}

// TODO: fix: currently out of ./build.sh format path
