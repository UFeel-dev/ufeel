#include "SpeechToText/SpeechRecognizer.hpp"
#include "vosk_api.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

class FakeVosk
{
public:

    static inline bool modelCreationSucceeds = true;
    static inline bool recognizerCreationSucceeds = true;
    static inline bool finalizedResultIsNull = false;

    static inline int modelHandle = 1;
    static inline int recognizerHandle = 2;

    static inline bool modelFree = false;
    static inline bool recognizerFree = false;
    static inline bool recognizerReset = false;

    static inline int waveformResult = 0;

    static inline std::string finalResult = R"({"text": "hello world"})";
    static inline std::string partialResult = R"({"partial": "hello"})";
    static inline std::string finalizedResult = R"({"text": "final sentence"})";

    static inline int lastSampleCount = 0;

    static auto model() -> VoskModel*
    {
        return reinterpret_cast<VoskModel*>(&modelHandle);
    }

    static auto recognizer() -> VoskRecognizer*
    {
        return reinterpret_cast<VoskRecognizer*>(&recognizerHandle);
    }

    static void reset()
    {
        modelCreationSucceeds = true;
        recognizerCreationSucceeds = true;

        modelFree = false;
        recognizerFree = false;
        recognizerReset = false;

        waveformResult = 0;

        finalResult = R"({"text": "hello world"})";
        partialResult = R"({"partial": "hello"})";
        finalizedResult = R"({"text": "final sentence"})";

        lastSampleCount = 0;
    }
};

void vosk_model_free(
    VoskModel*
)
{
    FakeVosk::modelFree = true;
}

void vosk_recognizer_free(
    VoskRecognizer*
)
{
    FakeVosk::recognizerFree = true;
}

VoskModel* vosk_model_new(
    const char*
)
{
    if (!FakeVosk::modelCreationSucceeds)
    {
        return nullptr;
    }

    return FakeVosk::model();
}

VoskRecognizer* vosk_recognizer_new(
    VoskModel*, float
)
{
    if (!FakeVosk::recognizerCreationSucceeds)
    {
        return nullptr;
    }

    return FakeVosk::recognizer();
}

int vosk_recognizer_accept_waveform(
    VoskRecognizer*, const char*, int length
)
{
    FakeVosk::lastSampleCount = length;
    return FakeVosk::waveformResult;
}

const char* vosk_recognizer_result(
    VoskRecognizer*
)
{
    return FakeVosk::finalResult.c_str();
}

const char* vosk_recognizer_partial_result(
    VoskRecognizer*
)
{
    return FakeVosk::partialResult.c_str();
}

const char* vosk_recognizer_final_result(
    VoskRecognizer*
)
{
    if (FakeVosk::finalizedResultIsNull)
    {
        return nullptr;
    }

    return FakeVosk::finalizedResult.c_str();
}

void vosk_recognizer_reset(
    VoskRecognizer*
)
{
    FakeVosk::recognizerReset = true;
}

class TestSpeechRecognizer : public ::testing::Test
{
protected:

    void SetUp() override
    {
        FakeVosk::reset();
    }
};

TEST_F(
    TestSpeechRecognizer, ConstructorSucceeds
)
{
    EXPECT_NO_THROW(SpeechRecognizer recognizer("fake-model-path"));
}

TEST_F(
    TestSpeechRecognizer, ConstructorThrowsWhenModelCreationFails
)
{
    FakeVosk::modelCreationSucceeds = false;

    EXPECT_THROW(SpeechRecognizer recognizer("invalid-model-path"), std::runtime_error);

    EXPECT_FALSE(FakeVosk::modelFree);
}

TEST_F(
    TestSpeechRecognizer, ConstructorThrowsWhenRecognizerCreationFails
)
{
    FakeVosk::recognizerCreationSucceeds = false;

    EXPECT_THROW(SpeechRecognizer recognizer("fake-model-path"), std::runtime_error);

    EXPECT_TRUE(FakeVosk::modelFree);
}

TEST_F(
    TestSpeechRecognizer, DestructorFreesRecognizerAndModel
)
{
    {
        SpeechRecognizer recognizer("fake-model-path");
    }

    EXPECT_TRUE(FakeVosk::recognizerFree);
    EXPECT_TRUE(FakeVosk::modelFree);
}

TEST_F(
    TestSpeechRecognizer, ResetCallsVoskReset
)
{
    SpeechRecognizer recognizer("fake-model-path");

    recognizer.reset();

    EXPECT_TRUE(FakeVosk::recognizerReset);
}

TEST_F(
    TestSpeechRecognizer, ProcessReturnsPartialText
)
{
    const int16_t samples[] = {100, 200, 300};

    SpeechRecognizer recognizer("fake-model-path");

    FakeVosk::waveformResult = 0;

    EXPECT_EQ(recognizer.process(samples, 3), "hello");

    EXPECT_EQ(FakeVosk::lastSampleCount, 3 * sizeof(int16_t));
}

TEST_F(
    TestSpeechRecognizer, ProcessReturnsFinalText
)
{
    const int16_t samples[] = {100, 200, 300};

    SpeechRecognizer recognizer("fake-model-path");

    FakeVosk::waveformResult = 1;

    EXPECT_EQ(recognizer.process(samples, 3), "hello world");
}

TEST_F(
    TestSpeechRecognizer, ProcessReturnsEmptyTextWhenJsonKeyIsMissing
)
{
    FakeVosk::partialResult = R"({"other": "value"})";

    const int16_t samples[] = {100};

    SpeechRecognizer recognizer("fake-model-path");

    FakeVosk::waveformResult = 0;

    EXPECT_TRUE(recognizer.process(samples, 1).empty());
}

TEST_F(
    TestSpeechRecognizer, FinalizeReturnsEmptyTextWhenJsonIsNull
)
{
    FakeVosk::finalizedResultIsNull = true;

    SpeechRecognizer recognizer("fake-model-path");

    EXPECT_TRUE(recognizer.finalize().empty());
}
