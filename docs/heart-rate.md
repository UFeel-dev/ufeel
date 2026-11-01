# Heart Rate

UFeel reads heart rate from a smartwatch or a dedicated sensor.

## Smartwatch

1. Install the [ufeel-watch](https://github.com/UFeel-dev/ufeel-watch) app on your watch.
2. Connect the watch and your computer to the same Wi-Fi.
3. Open UDP port `9999` on your computer:

```bash
sudo ufw allow 9999/udp                                                           # Ubuntu
sudo firewall-cmd --permanent --add-port=9999/udp && sudo firewall-cmd --reload   # Fedora
```

1. Tap **Start** in the watch app and keep the watch on your wrist. The first reading usually takes 10-20 seconds.

The watch sends the BPM as plain text (e.g. `72`) in a UDP packet to port `9999`. There is no authentication or encryption yet: use trusted networks only.

## Dedicated sensor

A local HTTP server at `http://localhost:8000` must answer with:

```json
{"bpm": 72}
```

## Choosing a source

| Value | Behavior |
| --- | --- |
| `Auto` (default) | Uses the sensor if available, otherwise the watch |
| `Sensor` | Dedicated sensor only |
| `Watch` | Smartwatch only |

```cpp
DataProcessor processor(modelDirectory, speechModelPath, HeartRateSourceType::Watch);
```

The C API always uses `Auto`.

## Status

`bpm` is only valid when `status` is `Valid`.

| Status | Value | Meaning |
| --- | --- | --- |
| `Valid` | 3 | Fresh reading |
| `NoData` | 4 | Nothing received, or last reading older than 5 seconds |
| `JsonParseError`, `JsonParseOutOfRange`, `RequestFailed` | 0-2 | Sensor errors (invalid JSON, missing `bpm`, request failed) |

## Troubleshooting

Always `NoData` with the watch:

- Check that UDP port `9999` is open and both devices are on the same network.
- If your router blocks broadcast, set `TARGET` to your computer's IP in the watch app.
- Test the receiver without the watch: `echo 72 | nc -u -w0 127.0.0.1 9999`
