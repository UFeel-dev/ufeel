# Tools

- simulate_heartbeat_server: Runs a web server to simulate the heartbeat when you cannot use a working sensor.
- test-ci-with-act.sh: Runs the CI workflow locally with [act](https://nektosact.com) on a copy of the working tree in `ufeel-act/` (git-ignored), then cleans it with `./build.sh clean`. Requires `act` and Docker.
