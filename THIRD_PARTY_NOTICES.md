# Third-Party Notices

UFEEL native library is licensed under the Apache License 2.0 (see [LICENSE](LICENSE), Copyright 2026 augustin-grosnon).

This file lists the third-party components distributed with or linked by the library, with their licenses. 

## Libraries

| Component | Version | License | License file |
| --- | --- | --- | --- |
| yaml-cpp | 0.7.0 | MIT | `3rdparty/yaml-cpp/LICENSE` and googletest 1.10.0 (test only, not built): `3rdparty/yaml-cpp/test/gtest-1.10.0/LICENSE` |
| nlohmann/json | 3.12.0 | MIT | `3rdparty/json/share/licenses/json/LICENSE.MIT` |
| cpr | 1.15.1 | MIT | `3rdparty/cpr/share/licenses/cpr/LICENSE` |
| OpenCV | 4.8.1 | Apache-2.0 | `3rdparty/opencv/share/licenses/opencv4/LICENSE` |
| OpenCV bundled libs | 4.8.1 | See `3rdparty/opencv/share/licenses/opencv4/` (zlib, libpng, libjpeg-turbo, libtiff, OpenEXR, protobuf, flatbuffers, ittnotify, quirc, and others) | `3rdparty/opencv/share/licenses/opencv4/` |
| LibTorch | 2.10.0 | BSD-3-Clause | `3rdparty/libtorch/share/licenses/libtorch/LICENSE`, `3rdparty/libtorch/share/licenses/libtorch/NOTICE` |
| Vosk | - | Apache-2.0 | `3rdparty/vosk/share/licenses/vosk/LICENSE` |
| Kaldi (in Vosk) | 5.5.1095~1-bc5ba (git `bc5baf1`) | Apache-2.0 | `3rdparty/vosk/share/licenses/kaldi/COPYING` |
| OpenFST (in Vosk) | - | Apache-2.0 | - |
| OpenBLAS (in Vosk) | 0.3.20 | BSD-3-Clause | `3rdparty/vosk/share/licenses/openblas/LICENSE` |
| libf2c (in Vosk) | - | Netlib / HPND-style | - |
| TensorFlow Lite | - | Apache-2.0 | `3rdparty/tensorflowlite/share/licenses/tensorflowlite/LICENSE` |
| FlatBuffers (in TFLite) | 23.5.26 (TFLite headers) | Apache-2.0 | `3rdparty/opencv/share/licenses/opencv4/flatbuffers-LICENSE.txt` and TFLite headers |
| PortAudio | - | MIT-style | `3rdparty/portaudio/share/licenses/portaudio/LICENSE.txt` |

## Models

| Model | Path | License |
| --- | --- | --- |
| vosk-model-small-de-0.15 | `models/SpeechToText/vosk-model-small-de-0.15/` | Apache-2.0 |
| vosk-model-small-fr-0.22 | `models/SpeechToText/vosk-model-small-fr-0.22/` | Apache-2.0 |
| vosk-model-small-en-us-0.15 | `models/SpeechToText/vosk-model-small-en-us-0.15/` | Apache-2.0 |
| YuNet face detection (`face_detection_yunet_2023mar.onnx`) | `models/Emotions/` | Apache-2.0 |
| MediaPipe face/iris models (`face_detection_short.tflite`, `face_landmark.tflite`, `iris_landmark.tflite`) | `models/EyeTracking/` | Apache-2.0 |
| binary_affectnet_resnet34_pretrained.pt / binary_affectnet_resnet34_pretrained.yaml / model_weights.bin | `models/Emotions/` | MIT |
