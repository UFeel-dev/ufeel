# Testing

> [<- Back to README](../README.md)

Tests use GoogleTest (C++) and .NET (C#). Run them inside the [Docker environment](../docker/README.md).

| Level | Command | Location | Scope |
| --- | --- | --- | --- |
| Unit | `./build.sh test-unit` | [tests/unit/cpp](../tests/unit/cpp) | Library and Wrapper classes in isolation. |
| Integration | `./build.sh test-integration` | [tests/integration](../tests/integration) | C++ `DataProcessor`/wrapper tests, plus C# P/Invoke tests ([UFeel.FeatureTests](../tests/integration/csharp/UFeel.FeatureTests)). |
| E2E | `./build.sh test-e2e` | [tests/e2e](../tests/e2e) | Full C API flow. |
| Feature matrix | `./build.sh test-feature-matrix` | `package/matrix` | C# tests against every feature combination. |
| All | `./build.sh test` | | Unit + integration + e2e. |
| Coverage | `./build.sh coverage` | | Native tests with coverage report. |

## Rules

- Unit tests mirror the source tree: `Library/src/X/Y.cpp` is tested in `tests/unit/cpp/Library/X/TestY.cpp`, and `Wrapper/src/X.cpp` in `tests/unit/cpp/Wrapper/src/TestX.cpp`.
- Integration and e2e C++ tests only build when all four features are enabled.
- Some tests need a camera and a microphone.
- Test images and their licenses: [tests/resources/README.md](../tests/resources/README.md).

## Manual testing

`./build.sh test-harness` builds an interactive Avalonia app ([UFeel.TestHarness](../tests/test-harnesses/csharp/UFeel.TestHarness)) to try every feature live. For heart rate without hardware, use [Tools/](../Tools/TOOLS.md).
