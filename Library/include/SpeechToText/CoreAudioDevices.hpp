#ifndef SPEECHTOTEXT_COREAUDIODEVICES_HPP
#define SPEECHTOTEXT_COREAUDIODEVICES_HPP

#include <functional>

/** Returns true once after CoreAudio adds or removes a device, or changes the default input. */
auto watchCoreAudioDevices() -> std::function<bool()>;

#endif // SPEECHTOTEXT_COREAUDIODEVICES_HPP
