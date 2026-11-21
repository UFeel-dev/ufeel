#ifndef TESTS_UNIT_LIBRARY_SPEECHTOTEXT_TESTDOUBLES_HPP // NOLINT(llvm-header-guard)
#define TESTS_UNIT_LIBRARY_SPEECHTOTEXT_TESTDOUBLES_HPP

#include "portaudio.h"

#include <string>
#include <vector>

namespace speech_to_text_test
{

struct FakeInputDevice
{
    std::string name = "Fake input device";
    int inputChannels = 2;
    std::vector<double> sampleRates; // * empty: every rate is supported
};

/** Channel c of every captured frame holds 100 + 200 * c. */
class PortAudioFake final
{
public:

    static void reset();
    static void setInitializeResult(PaError error);
    static void setDeviceCount(PaDeviceIndex count);
    static void setDevices(std::vector<FakeInputDevice> devices);
    static void setDefaultInputDevice(PaDeviceIndex device);
    static void setDeviceInfoAvailable(bool available);
    static void setFormatResult(PaError error);
    static void setOpenResult(PaError error);
    static void setStartResult(PaError error);
    static void setStopResult(PaError error);
    static void setCallbackInputAvailable(bool available);
    static void setCallbackFrameCount(unsigned long frameCount);
    static void invokeCallbackOnStop(bool enabled);

    static auto initializeCount() -> int;
    static auto openedDevice() -> PaDeviceIndex;
    static auto openedChannelCount() -> int;
    static auto openedSampleRate() -> double;
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
    static void setRecognizerCreationSucceeds(bool succeeds);
    static auto recognizerSampleRate() -> float;
};

} // namespace speech_to_text_test

#endif // TESTS_UNIT_LIBRARY_SPEECHTOTEXT_TESTDOUBLES_HPP
