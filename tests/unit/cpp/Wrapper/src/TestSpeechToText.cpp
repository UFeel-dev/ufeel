#include "ufeel.h"

#include <cstdlib>
#include <gtest/gtest.h>
#include <memory>

TEST(
    SpeechToTextApiTest, NullProcessorOperationsReturnInvalidProcessor
)
{
    EXPECT_EQ(ufeel_update_speech_to_text(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_set_speech_to_text_enabled(nullptr, 1), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_get_speech_to_text_result(nullptr, nullptr), UFEEL_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(ufeel_get_microphones_result(nullptr, nullptr), UFEEL_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(ufeel_set_microphone(nullptr, "microphone"), UFEEL_STATUS_INVALID_PROCESSOR);
}

TEST(
    SpeechToTextApiTest, MicrophonesGetterClearsOutputBeforeRejectingProcessor
)
{
    auto* result = ufeel_create_microphones_result();
    ASSERT_NE(result, nullptr);
    result->microphones = new UFeelMicrophone[1]{};
    result->microphones[0].name = static_cast<char*>(std::malloc(4));
    result->count = 1;

    EXPECT_EQ(ufeel_get_microphones_result(nullptr, result), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(result->microphones, nullptr);
    EXPECT_EQ(result->count, 0);
    ufeel_free_microphones_result(result);
    EXPECT_NO_THROW(ufeel_free_microphones_result(nullptr));
}

TEST(
    SpeechToTextApiTest, ResultGetterClearsOutputBeforeRejectingProcessor
)
{
    UFeelSpeechToTextResult result{};
    auto allocatedSpeech =
        std::unique_ptr<char, decltype(&std::free)>(static_cast<char*>(std::malloc(4)), &std::free);
    ASSERT_NE(allocatedSpeech, nullptr);
    result.speech = allocatedSpeech.release();
    result.status = UFEEL_SPEECH_STATUS_VALID;

    EXPECT_EQ(ufeel_get_speech_to_text_result(nullptr, &result), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(result.speech, nullptr);
    EXPECT_EQ(result.status, 0);
}

TEST(
    SpeechToTextApiTest, ResultAllocationAndReleaseAreNullSafe
)
{
    auto* result = ufeel_create_speech_to_text_result();

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->status, 0);
    EXPECT_EQ(result->speech, nullptr);
    ufeel_free_speech_to_text_result(result);
    EXPECT_NO_THROW(ufeel_free_speech_to_text_result(nullptr));
}
