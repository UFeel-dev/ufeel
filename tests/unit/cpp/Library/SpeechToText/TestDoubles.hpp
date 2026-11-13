#ifndef TESTS_UNIT_LIBRARY_SPEECHTOTEXT_TESTDOUBLES_HPP // NOLINT(llvm-header-guard)
#define TESTS_UNIT_LIBRARY_SPEECHTOTEXT_TESTDOUBLES_HPP

#include "portaudio.h"

#include <string>

namespace speech_to_text_test
{

class PortAudioFake final
{
public:

    static void reset();
    static void setInitializeResult(PaError error);
    static void setDeviceCount(PaDeviceIndex count);
    static void setDeviceInfoAvailable(bool available);
    static void returnNullDeviceInfoAfterFirstQuery(bool enabled);
    static void setDeviceCapabilities(int channels, double sampleRate, const char* name);
    static void setFormatResult(PaError error);
    static void setOpenResult(PaError error);
    static void setStartResult(PaError error);
    static void setStopResult(PaError error);
    static void setCallbackInputAvailable(bool available);
    static void setCallbackFrameCount(unsigned long frameCount);
    static void invokeCallbackOnStop(bool enabled);
};

class VoskFake final
{
public:

    static void reset();
    static void setFinalizedResult(const std::string& result);
    static void setFinalResult(const std::string& result);
    static void setPartialResult(const std::string& result);
    static void setResultNull(bool isNull);
    static void setPartialResultNull(bool isNull);
    static void setWaveformResult(int result);
};

} // namespace speech_to_text_test

#endif // TESTS_UNIT_LIBRARY_SPEECHTOTEXT_TESTDOUBLES_HPP
