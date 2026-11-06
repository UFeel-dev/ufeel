#include "ufeel.h"

#include <cstdlib>
#include <gtest/gtest.h>

TEST(
    SpeechToTextApiTest, NullProcessorOperationsReturnInvalidProcessor
)
{
    EXPECT_EQ(ufeel_update_speech_to_text(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_set_speech_to_text_enabled(nullptr, 1), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_get_speech_to_text_result(nullptr, nullptr), UFEEL_STATUS_INVALID_ARGUMENT);
}

TEST(
    SpeechToTextApiTest, ResultGetterClearsOutputBeforeRejectingProcessor
)
{
    UFeelSpeechToTextResult result{};
    result.speech = static_cast<char*>(std::malloc(4));
    ASSERT_NE(result.speech, nullptr);
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
