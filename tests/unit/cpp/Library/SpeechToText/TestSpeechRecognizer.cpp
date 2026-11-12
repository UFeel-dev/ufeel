#include "SpeechToText/SpeechRecognizer.hpp"
#include "TestDoubles.hpp"
#include "vosk_api.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

namespace
{

struct FakeVoskState
{
    bool modelCreationSucceeds = true;
    bool recognizerCreationSucceeds = true;
    bool finalizedResultIsNull = false;
    bool resultIsNull = false;
    bool partialResultIsNull = false;

    int modelHandle = 1;
    int recognizerHandle = 2;

    bool modelFree = false;
    bool recognizerFree = false;
    bool recognizerReset = false;

    int waveformResult = 0;

    std::string finalResult = R"({"text": "hello world"})";
    std::string partialResult = R"({"partial": "hello"})";
    std::string finalizedResult = R"({"text": "final sentence"})";

    int lastSampleCount = 0;
    std::atomic<bool> audioProcessed = false;
};

auto fakeVoskState() -> FakeVoskState&
{
    static FakeVoskState state;
    return state;
}

auto fakeModel() -> VoskModel*
{
    return reinterpret_cast<VoskModel*>(&fakeVoskState().modelHandle);
}

auto fakeRecognizer() -> VoskRecognizer*
{
    return reinterpret_cast<VoskRecognizer*>(&fakeVoskState().recognizerHandle);
}

} // namespace

namespace speech_to_text_test
{

void VoskFake::reset()
{
    FakeVoskState& state = fakeVoskState();
    state.modelCreationSucceeds = true;
    state.recognizerCreationSucceeds = true;
    state.finalizedResultIsNull = false;
    state.resultIsNull = false;
    state.partialResultIsNull = false;
    state.modelFree = false;
    state.recognizerFree = false;
    state.recognizerReset = false;
    state.waveformResult = 0;
    state.finalResult = R"({"text": "hello world"})";
    state.partialResult = R"({"partial": "hello"})";
    state.finalizedResult = R"({"text": "final sentence"})";
    state.lastSampleCount = 0;
    state.audioProcessed.store(false);
}

void VoskFake::setFinalizedResult(
    const std::string& result
)
{
    fakeVoskState().finalizedResult = result;
    fakeVoskState().finalizedResultIsNull = false;
}

void VoskFake::setFinalResult(
    const std::string& result
)
{
    fakeVoskState().finalResult = result;
    fakeVoskState().resultIsNull = false;
}

void VoskFake::setPartialResult(
    const std::string& result
)
{
    fakeVoskState().partialResult = result;
    fakeVoskState().partialResultIsNull = false;
}

void VoskFake::setResultNull(
    bool isNull
)
{
    fakeVoskState().resultIsNull = isNull;
}

void VoskFake::setPartialResultNull(
    bool isNull
)
{
    fakeVoskState().partialResultIsNull = isNull;
}

void VoskFake::setWaveformResult(
    int result
)
{
    fakeVoskState().waveformResult = result;
}

auto VoskFake::hasProcessedAudio() -> bool
{
    return fakeVoskState().audioProcessed.load();
}

} // namespace speech_to_text_test

void vosk_model_free(
    VoskModel* model
)
{
    (void)model;
    fakeVoskState().modelFree = true;
}

void vosk_recognizer_free(
    VoskRecognizer* recognizer
)
{
    (void)recognizer;
    fakeVoskState().recognizerFree = true;
}

auto vosk_model_new(
    const char* model_path // NOLINT(readability-identifier-naming)
) -> VoskModel*
{
    (void)model_path;
    if (!fakeVoskState().modelCreationSucceeds)
    {
        return nullptr;
    }

    return fakeModel();
}

auto vosk_recognizer_new(
    VoskModel* model, float sample_rate // NOLINT(readability-identifier-naming)
) -> VoskRecognizer*
{
    (void)model;
    (void)sample_rate;
    if (!fakeVoskState().recognizerCreationSucceeds)
    {
        return nullptr;
    }

    return fakeRecognizer();
}

auto vosk_recognizer_accept_waveform(
    VoskRecognizer* recognizer, const char* data, int length
) -> int
{
    (void)recognizer;
    (void)data;
    fakeVoskState().lastSampleCount = length;
    fakeVoskState().audioProcessed = true;
    return fakeVoskState().waveformResult;
}

auto vosk_recognizer_result(
    VoskRecognizer* recognizer
) -> const char*
{
    (void)recognizer;
    if (fakeVoskState().resultIsNull)
    {
        return nullptr;
    }

    return fakeVoskState().finalResult.c_str();
}

auto vosk_recognizer_partial_result(
    VoskRecognizer* recognizer
) -> const char*
{
    (void)recognizer;
    if (fakeVoskState().partialResultIsNull)
    {
        return nullptr;
    }

    return fakeVoskState().partialResult.c_str();
}

auto vosk_recognizer_final_result(
    VoskRecognizer* recognizer
) -> const char*
{
    (void)recognizer;
    if (fakeVoskState().finalizedResultIsNull)
    {
        return nullptr;
    }

    return fakeVoskState().finalizedResult.c_str();
}

void vosk_recognizer_reset(
    VoskRecognizer* recognizer
)
{
    (void)recognizer;
    fakeVoskState().recognizerReset = true;
}

namespace
{

class TestSpeechRecognizer : public ::testing::Test
{
protected:

    void SetUp() override
    {
        speech_to_text_test::VoskFake::reset();
    }
};

TEST_F(
    TestSpeechRecognizer, ConstructorSucceeds
)
{
    EXPECT_NO_THROW(const SpeechRecognizer recognizer("fake-model-path"));
}

TEST_F(
    TestSpeechRecognizer, ConstructorThrowsWhenModelCreationFails
)
{
    fakeVoskState().modelCreationSucceeds = false;

    EXPECT_THROW(const SpeechRecognizer recognizer("invalid-model-path"), std::runtime_error);

    EXPECT_FALSE(fakeVoskState().modelFree);
}

TEST_F(
    TestSpeechRecognizer, ConstructorThrowsWhenRecognizerCreationFails
)
{
    fakeVoskState().recognizerCreationSucceeds = false;

    EXPECT_THROW(const SpeechRecognizer recognizer("fake-model-path"), std::runtime_error);

    EXPECT_TRUE(fakeVoskState().modelFree);
}

TEST_F(
    TestSpeechRecognizer, DestructorFreesRecognizerAndModel
)
{
    {
        const SpeechRecognizer recognizer("fake-model-path");
    }

    EXPECT_TRUE(fakeVoskState().recognizerFree);
    EXPECT_TRUE(fakeVoskState().modelFree);
}

TEST_F(
    TestSpeechRecognizer, ResetCallsVoskReset
)
{
    SpeechRecognizer recognizer("fake-model-path");

    recognizer.reset();

    EXPECT_TRUE(fakeVoskState().recognizerReset);
}

TEST_F(
    TestSpeechRecognizer, ProcessReturnsPartialText
)
{
    constexpr std::array<int16_t, 3> samples{100, 200, 300};

    SpeechRecognizer recognizer("fake-model-path");

    fakeVoskState().waveformResult = 0;

    EXPECT_EQ(recognizer.process(samples.data(), samples.size()), "hello");

    EXPECT_EQ(fakeVoskState().lastSampleCount, 3 * sizeof(int16_t));
}

TEST_F(
    TestSpeechRecognizer, ProcessReturnsFinalText
)
{
    constexpr std::array<int16_t, 3> samples{100, 200, 300};

    SpeechRecognizer recognizer("fake-model-path");

    fakeVoskState().waveformResult = 1;

    EXPECT_EQ(recognizer.process(samples.data(), samples.size()), "hello world");
}

TEST_F(
    TestSpeechRecognizer, ProcessReturnsEmptyTextWhenJsonKeyIsMissing
)
{
    fakeVoskState().partialResult = R"({"other": "value"})";

    constexpr std::array<int16_t, 1> samples{100};

    SpeechRecognizer recognizer("fake-model-path");

    fakeVoskState().waveformResult = 0;

    EXPECT_TRUE(recognizer.process(samples.data(), samples.size()).empty());
}

TEST_F(
    TestSpeechRecognizer, ProcessReturnsEmptyTextForNullOrMalformedJson
)
{
    constexpr std::array<int16_t, 1> samples{100};
    SpeechRecognizer recognizer("fake-model-path");

    speech_to_text_test::VoskFake::setPartialResultNull(true);
    EXPECT_TRUE(recognizer.process(samples.data(), samples.size()).empty());

    speech_to_text_test::VoskFake::setPartialResult(R"({"partial": )");
    EXPECT_TRUE(recognizer.process(samples.data(), samples.size()).empty());

    speech_to_text_test::VoskFake::setPartialResult(R"({"partial": "unfinished)");
    EXPECT_TRUE(recognizer.process(samples.data(), samples.size()).empty());

    speech_to_text_test::VoskFake::setWaveformResult(1);
    speech_to_text_test::VoskFake::setResultNull(true);
    EXPECT_TRUE(recognizer.process(samples.data(), samples.size()).empty());
}

TEST_F(
    TestSpeechRecognizer, FinalizeReturnsEmptyTextWhenJsonIsNull
)
{
    fakeVoskState().finalizedResultIsNull = true;

    SpeechRecognizer recognizer("fake-model-path");

    EXPECT_TRUE(recognizer.finalize().empty());
}

} // namespace
