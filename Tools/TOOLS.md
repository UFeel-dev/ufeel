# Tools

## Simulate HeartBeat

- `simulate_heartbeat_server.py`: Runs a web server to simulate the heartbeat when you cannot use a working sensor. It answers every `GET` on `localhost:8000` with a random `{"bpm": N}` between 50 and 200.

  ```bash
  python3 Tools/simulate_heartbeat_server.py
  ```

## Test CI

- `test-ci-with-act.sh`: Runs CI jobs locally with [act](https://nektosact.com) on a copy of the working tree in `ufeel-act/` (git-ignored). Requires `act`, Docker, and Git LFS.

  ```bash
  Tools/test-ci-with-act.sh [lint|build|release]...
  ```

  - Runs the given jobs in order, `lint build` by default.
  - `lint` and `build` run as a pull request to `main`.
  - `release` runs as a push to `release`: it reads `VERSION` (and fails if its tag already exists on GitHub), builds, and leaves the archive in `ufeel-act/`. It never creates a tag or a release, because the publish step is skipped under act.
  - Like the CI checkout, Git LFS files are copied as pointers. The jobs fetch them from the local LFS objects, hard-linked into the copy, so nothing is downloaded.
  - Afterwards it runs `./build.sh clean` and gives the copy back to your user.
