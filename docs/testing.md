# Testing

> [<- Back to README](../README.md)

Tests use GoogleTest (C++) and .NET (C#). Run them inside the [Docker environment](../docker/README.md).

| Level | Command | Location | Scope |
| --- | --- | --- | --- |
| Unit | `./build.sh test-unit` | [tests/unit/cpp](../tests/unit/cpp) | Library and Wrapper classes in isolation. |
| Integration | `./build.sh test-integration` | [tests/integration](../tests/integration) | `DataProcessor` with real models on [fixtures](../tests/resources), wrapper tests, plus C# P/Invoke tests ([UFeel.FeatureTests](../tests/integration/csharp/UFeel.FeatureTests)). |
| E2E | `./build.sh test-e2e` | [tests/e2e](../tests/e2e) | Full C API flow with the real camera, microphone and heart-rate ports. |
| Feature matrix | `./build.sh test-feature-matrix` | `package/matrix` | C# tests against every feature combination. |
| All | `./build.sh test` | | Every test except the `hardware` group. Runs in CI. |
| Coverage | `./build.sh coverage` | | Native tests with coverage report. |

## Hardware

Tests labelled `hardware` (the e2e tests) use the real camera, microphone and the fixed heart-rate ports `8000` and `9999`. Run them manually with `./build.sh test-e2e` or `ctest -L hardware`.

Every other test must pass without devices or network. Build `DataProcessor` with `DataProcessorDevices` to replace each device:

| Device | Test replacement |
| --- | --- |
| Camera | `frameReader`, for example `frameSequence()` from [Fixtures.hpp](../tests/unit/cpp/include/Library/TestSupport/Fixtures.hpp) |
| Microphone | `audioSource`, for example [WavAudioSource](../tests/unit/cpp/include/Library/SpeechToText/TestSupport/WavAudioSource.hpp) |
| Heart rate | `heartRateSources`, for example [HeartRateSourceFake](../tests/unit/cpp/include/Library/HeartRate/TestSupport/HeartRateSourceFake.hpp), or real sources on ephemeral ports (`WatchHeartRateSource(0)`, [LocalHttpServer](../tests/unit/cpp/include/Library/HeartRate/TestSupport/LocalHttpServer.hpp)) |
| Clock | Pass a time point to `LatestReading`, `SpeechToTextDetector::process`, `EyeTrackingDetector::process` and `setCalibrationTarget` instead of sleeping |

## Rules

- Unit tests mirror the source tree: `Library/src/X/Y.cpp` is tested in `tests/unit/cpp/Library/X/TestY.cpp`, and `Wrapper/src/X.cpp` in `tests/unit/cpp/Wrapper/src/TestX.cpp`.
- Integration and e2e C++ tests only build when all four features are enabled.
- Assert on the known answer of a fixture instead of skipping: a face, a blank frame, a grayscale frame, a small face, a spoken sentence, silence.
- Test resources and their licenses: [tests/resources/README.md](../tests/resources/README.md).

## Manual testing

`./build.sh test-harness` builds an interactive Avalonia app ([UFeel.TestHarness](../tests/test-harnesses/csharp/UFeel.TestHarness)) to try every feature live. For heart rate without hardware, use [Tools/](../Tools/TOOLS.md).
