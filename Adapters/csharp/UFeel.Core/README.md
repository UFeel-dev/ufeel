# UFeel.Core (C# adapter)

Engine-agnostic C# binding for `libufeel_wrapper`. Limited to C# 9 / netstandard2.1, so it compiles as-is in Unity 6 and Godot .NET.

## Install

- **Unity:** copy this folder into a package. `UFeel.Core.asmdef` defines the assembly.
- **Other .NET hosts:** compile the folder, or reference `UFeel.Core.csproj` from the ufeel repo.
- `libufeel_wrapper` must be loadable (engine plugin, `NativeLibrary.SetDllImportResolver` or `LD_LIBRARY_PATH`), with `dependencies/` next to it.

## Use

```csharp
var processor = new UFeelProcessor(UFeelPackageLayout.FromRoot(packageDirectory));
var profiles = CalibrationProfileStore.CreateDefault();
profiles.Load(processor);                       // before Start()

var session = new UFeelSession(processor);      // owns and disposes the processor
session.Start();

session.Demand.Request(DetectionType.Emotions); // each frame you read it (or Demand.Pin once)
EmotionType? dominant = session.Latest.Emotions.Dominant;

IReadOnlyList<MicrophoneDevice> microphones = await session.GetMicrophonesAsync();
await session.SelectMicrophoneAsync(microphones[0].Name); // null: default microphone
await session.SetSpeechToTextEnabledAsync(true);
await new EyeTrackingCalibration(session, presenter, profiles).RunAsync(token);

session.Dispose();
```

## Components

| Type | Role |
| --- | --- |
| `UFeelProcessor` | Owns the native handle. Not thread-safe. |
| `UFeelSession` | Background thread running the processor. Publishes `Latest` (immutable, lock-free) and runs commands through `ExecuteAsync`. |
| `FeatureDemand` | Selects the features that run: pinned, or requested within the last 2 s. |
| `FeatureScheduler` | Per-feature intervals, one image feature per frame. |
| `EyeTrackingCalibration` | Guided calibration through an `ICalibrationPresenter`. |
| `CalibrationProfileStore` | Profile file, shared by every host by default. |
| `RuleSet` | Condition to action rules, evaluated on the host's main thread. |
| `UFeelText` | Display strings for results. |

## Rules

- After `session.Start()`, reach the processor only through `session.ExecuteAsync`.
- Keep to C# 9 / netstandard2.1 (which can be checked with `dotnet build UFeel.Core.csproj`).
