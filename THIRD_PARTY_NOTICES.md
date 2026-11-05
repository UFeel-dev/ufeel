# Third-Party Notices

UFEEL native library is licensed under the Apache License 2.0 (see [LICENSE](LICENSE), Copyright 2026 augustin-grosnon).

This file lists the third-party components distributed with or linked by the library, with their licenses. License texts are stored in each component's `share/licenses/` directory, following the OpenCV layout (`3rdparty/opencv/share/licenses/opencv4/`).

## Libraries

| Component | Version | License | Copyright | License file | Status |
| --- | --- | --- | --- | --- | --- |
| yaml-cpp | 0.7.0 | MIT | Copyright (c) 2008-2015 Jesse Beder | `3rdparty/yaml-cpp/LICENSE` (submodule, not modified) | Verified |
| nlohmann/json | 3.12.0 | MIT | Copyright (c) 2013-2026 Niels Lohmann | `3rdparty/json/share/licenses/json/LICENSE.MIT` | Verified |
| cpr | — | MIT | Copyright (c) 2017-2021 Huu Nguyen; Copyright (c) 2022 libcpr and many other contributors | `3rdparty/cpr/share/licenses/cpr/LICENSE` | Verified |
| OpenCV | 4.8.1 | Apache-2.0 | Not stated in the Apache-2.0 text; see license file | `3rdparty/opencv/share/licenses/opencv4/LICENSE` | Verified |
| OpenCV bundled libs | 4.8.1 | See `3rdparty/opencv/share/licenses/opencv4/` (zlib, libpng, libjpeg-turbo, libtiff, OpenEXR, protobuf, flatbuffers, ittnotify, quirc, and others) | Per file | `3rdparty/opencv/share/licenses/opencv4/` | Shipped by OpenCV |
| LibTorch | 2.10.0 | BSD-3-Clause | Copyright (c) 2016- Facebook, Inc (Adam Paszke); Copyright (c) 2014- Facebook, Inc (Soumith Chintala); Copyright (c) 2011-2014 Idiap Research Institute (Ronan Collobert) | `3rdparty/libtorch/share/licenses/libtorch/LICENSE`, `3rdparty/libtorch/share/licenses/libtorch/NOTICE` | Verified |
| Vosk | non pinnée | Apache-2.0 | Unverified (the en-us model README states "Copyright 2020 Alpha Cephei Inc") | `3rdparty/vosk/share/licenses/vosk/LICENSE` | To pin (text from `master`) |
| Kaldi (in Vosk) | non pinnée | Apache-2.0 | Kaldi contributors (see per-file headers) | `3rdparty/vosk/share/licenses/kaldi/COPYING` | To pin (text from `master`) |
| OpenFST (in Vosk) | 1.8.3 (source installed version unknown) | Apache-2.0 | Copyright 2005-2020 Google LLC | `3rdparty/vosk/share/licenses/openfst/LICENSE` (full Apache-2.0 text from openfst repository, plus copyright line) | To pin (installed version unknown) |
| OpenBLAS (in Vosk) | 0.3.20 | BSD-3-Clause | Copyright (c) 2011-2014, The OpenBLAS Project | `3rdparty/vosk/share/licenses/openblas/LICENSE` | To pin (text from `develop`) |
| libf2c (in Vosk) | — | Netlib / HPND-style | See netlib f2c sources | Not retrieved | Missing |
| TensorFlow Lite | version unknown | Apache-2.0 | Not stated in the Apache-2.0 text; see license file | `3rdparty/tensorflowlite/share/licenses/tensorflowlite/LICENSE` | To pin (text from `master`) |
| FlatBuffers (in TFLite) | — | Apache-2.0 | Copyright 2014-2022 Google Inc. (headers) | `3rdparty/opencv/share/licenses/opencv4/flatbuffers-LICENSE.txt` and TFLite headers | Verified in headers |
| PortAudio | non pinnée | MIT-style | Copyright (c) 1999-2006 Ross Bencina and Phil Burk | `3rdparty/portaudio/share/licenses/portaudio/LICENSE.txt` | Verified in headers, text from `master` |

## Models

| Model | Path | License | Source of license | Status |
| --- | --- | --- | --- | --- |
| vosk-model-small-de-0.15 | `models/SpeechToText/vosk-model-small-de-0.15/` | Apache-2.0 | `COPYING` in the model directory | Verified |
| vosk-model-small-fr-0.22 | `models/SpeechToText/vosk-model-small-fr-0.22/` | Apache-2.0 | `COPYING` in the model directory | Verified on website |
| vosk-model-small-en-us-0.15 | `models/SpeechToText/vosk-model-small-en-us-0.15/` | Apache-2.0 | `COPYING` in the model directory | Verified on website |
| YuNet face detection (`face_detection_yunet_2023mar.onnx`) | `models/Emotions/` | Apache-2.0 | opencv_zoo repository license: `models/Emotions/LICENSE.yunet` | Repo-level, not model-specific |
| MediaPipe face/iris models (`face_detection_short.tflite`, `face_landmark.tflite`, `iris_landmark.tflite`) | `models/EyeTracking/` | Apache-2.0 | MediaPipe repository license: `models/EyeTracking/LICENSE.mediapipe` | Repo-level, not model-specific |
| binary_affectnet_resnet34_pretrained.pt / binary_affectnet_resnet34_pretrained.yaml / model_weights.bin | `models/Emotions/` | MIT | Provided by the project owner (assumption, not checked against a source in the repo) | Assumed |
