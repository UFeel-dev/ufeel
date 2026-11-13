# Tools

- simulate_heartbeat_server: Runs a web server to simulate the heartbeat when you cannot use a working sensor.
- test-ci-with-act.sh: Runs the CI workflow locally with [act](https://nektosact.com) on a copy of the working tree in `ufeel-act/` (git-ignored). Like the CI checkout, Git LFS files are copied as pointers. The build job fetches them from the local LFS objects, hard-linked into the copy, so nothing is downloaded. Afterwards it runs `./build.sh clean` and gives the copy back to your user. Requires `act`, Docker, and Git LFS.
