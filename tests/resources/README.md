# Test resources

## astronaut.png

| | |
| --- | --- |
| **Description** | Color portrait photograph of Eileen Collins, an American NASA astronaut |
| **Used for** | Face fixture of the emotion, eye tracking and `DataProcessor` tests. Greyscale, BGRA and small-face variants are derived from it in [Fixtures.hpp](../unit/cpp/include/Library/TestSupport/Fixtures.hpp) |
| **Original source** | NASA Great Images database, image ID GPN-2000-001177 |
| **Obtained via** | scikit-image, `skimage.data.astronaut()` |
| **Modifications** | None. Exported to PNG with `skimage.io.imsave` |

### License / usage rights

This image is a work of NASA and is distributed by scikit-image with
"no known copyright restrictions, released into the public domain".
It may be used, copied, and redistributed, including in open source and
commercial projects, without permission or fees.

This image is **not** covered by this repository's license. The repository's
license applies to the source code only.

### Usage notes

- NASA's media usage guidelines ask that NASA's name, emblem, and imagery not
  be used to suggest that NASA or the person pictured endorses this project.
  This project is not affiliated with or endorsed by NASA or Eileen Collins.
- The image is included only as a test fixture and is not used to identify
  or profile any individual.

### References

- scikit-image data documentation: <https://scikit-image.org/docs/stable/api/skimage.data.html>
- image downloaded from the NASA Great Images database: <https://www.flickr.com/photos/nasacommons/16504233985/>

## turn_on_the_light.wav

| | |
| --- | --- |
| **Description** | Synthetic voice saying "turn on the light", 16 kHz mono 16-bit PCM |
| **Used for** | Speech-to-text fixture of the `DataProcessor` integration tests (expected text: `turn on the light`) |
| **Original source** | Generated for this repository with eSpeak NG |
| **Modifications** | Resampled with SoX, 0.3 s of silence before and 0.8 s after |

Regenerate it with:

```bash
espeak-ng -v en-us -s 140 -w speech.wav "turn on the light"
sox speech.wav -r 16000 -c 1 -b 16 -e signed-integer turn_on_the_light.wav pad 0.3 0.8
```

### License / usage rights

The file is synthesized speech with no recorded person. It is provided under this repository's license.
