# AGENTS.md

UFeel: native C++ library (emotion recognition, eye tracking, speech to text, heart rate) with a C ABI. See the [README](README.md).

## Setup and build

- Environment and requirements: [README](README.md#quick-start), [Docker](docker/README.md) (recommended)
- Third-party libraries (Git LFS, submodules): [README](README.md#third-party-libraries)
- Commands: `./build.sh --help`, [README](README.md#build-and-test)
- Feature flags (`UFEEL_ENABLE_*`): [README](README.md#selecting-features), [Architecture](docs/architecture.md#feature-flags)
- Clang tooling setup: [README](README.md#quick-start)

## Codebase

- Layers, layout, adding a feature: [Architecture](docs/architecture.md)
- Public C API: [Wrapper/include/ufeel.h](Wrapper/include/ufeel.h)
- Heart rate sources: [docs/heart-rate.md](docs/heart-rate.md)
- Dev tools: [Tools/TOOLS.md](Tools/TOOLS.md)

## Quality

- Tests: [Testing](docs/testing.md)
- Format: `./build.sh format` ([.clang-format](.clang-format)), lint: `./build.sh lint` ([.clang-tidy](.clang-tidy))
- CI: [.github/workflows/ci.yml](.github/workflows/ci.yml)

## Contributing

- Process, coding standards, commit format: [CONTRIBUTING.md](CONTRIBUTING.md)
- PR template: [.github/PULL_REQUEST_TEMPLATE.md](.github/PULL_REQUEST_TEMPLATE.md)
- Reviewers: [CODEOWNERS.md](CODEOWNERS.md)
- Releases and tags: [VERSIONING.md](VERSIONING.md)

## Policies

- [Security](SECURITY.md) · [Disclaimer](DISCLAIMER.md) · [Code of conduct](CODE_OF_CONDUCT.md)
- Licenses: [LICENSE](LICENSE), [third-party notices](THIRD_PARTY_NOTICES.md)
