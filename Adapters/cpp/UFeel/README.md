# UFeel (C++ adapter)

C++20 binding for `libufeel_wrapper`, with the same components as the [C# adapter](../../csharp/UFeel.Core/README.md).

## Install

`./build.sh all --adapter cpp` packages this folder as `adapters/cpp/UFeel`. In CMake:

```cmake
add_subdirectory(path/to/package/adapters/cpp/UFeel ufeel)
target_link_libraries(my_app PRIVATE ufeel::ufeel)
```

`ufeel::ufeel` brings the headers and `libufeel_wrapper`. Outside the package, set `UFEEL_PACKAGE_DIR` to the package root. At runtime, `libufeel_wrapper.so` must be loadable, with `dependencies/` next to it.

## Use

```cpp
#include "ufeel/UFeel.hpp"

ufeel::Processor processor(ufeel::PackageLayout::fromRoot(packageDirectory, ufeel::SpeechLanguage::English));
const auto profiles = ufeel::CalibrationProfileStore::createDefault();
const ufeel::ProfileLoadResult profile = profiles.load(processor);

ufeel::Session session(std::move(processor), {.onMicrophonesChanged = refreshMicrophoneList});
session.start();

session.demand().request(ufeel::DetectionType::Emotions); // each frame you read it (or demand().pin once)
std::optional<ufeel::Emotion> dominant = session.latest()->emotions.dominant();

std::vector<ufeel::Microphone> microphones = session.microphones().get();
session.selectMicrophone(microphones.at(0).id); // "": default microphone
session.setSpeechToTextEnabled(true);

ufeel::EyeTrackingCalibration calibration(session, presenter, &profiles);
calibration.start();
std::optional<ufeel::CalibrationResult> result = calibration.update(); // each frame, until it has a value
```

## Components

| Type | Role |
| --- | --- |
| `Processor` | Native handle and results. Not thread-safe. |
| `Session` | Runs the processor on a thread: `latest()` snapshot, `execute()` commands. |
| `FeatureDemand` | Features that run: pinned, or requested in the last 2 s. |
| `FeatureScheduler` | Per-feature intervals, one image feature per frame. |
| `EyeTrackingCalibration` | Guided calibration, advanced by `update()`. |
| `CalibrationProfileStore` | Calibration profile file, shared with the C# adapter. |
| `PackageLayout` | Model paths and speech language of a package. |
| `RuleSet` | Condition to action rules, evaluated on the main thread. |
| `ufeel::text` | Display strings for results. |

## Rules

- Configure the processor (e.g. load the calibration profile) before the session takes it, then reach it only through `session.execute`.
- Session futures complete on the session thread: poll them with `wait_for(0s)`. They throw `std::future_error` if the session is destroyed first.
- `SessionOptions` callbacks run on the session thread: they must not block or throw.
- Call `EyeTrackingCalibration::update()` on the presenter's thread, and destroy the calibration before the session.
