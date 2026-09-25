# Versioning

UFeel follows [Semantic Versioning 2.0.0](https://semver.org/).

Versions use the format `MAJOR.MINOR.PATCH`.

Until `1.0.0`, UFeel is considered unstable:

* `MINOR` versions may contain breaking changes and new functionality.
* `PATCH` versions contain backward-compatible fixes.
* Pre-release versions use standard SemVer identifiers, such as `0.2.0-alpha.1`.

From `1.0.0` onward:

* `MAJOR` versions contain breaking changes.
* `MINOR` versions contain backward-compatible functionality.
* `PATCH` versions contain backward-compatible fixes.

The version applies to the public UFeel API and its compatibility. Internal implementation changes do not require a version change unless they affect the public API, ABI, or documented behavior.

Git tags use the `v` prefix, for example `v0.2.0` or `v0.2.0-alpha.1`.

Published version tags are annotated and must not be moved or reused.
