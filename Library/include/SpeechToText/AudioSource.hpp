#ifndef SPEECHTOTEXT_AUDIOSOURCE_HPP
#define SPEECHTOTEXT_AUDIOSOURCE_HPP

#include <cstdint>
#include <vector>

class AudioSource
{
public:

    AudioSource() = default;
    virtual ~AudioSource() = default;

    AudioSource(const AudioSource&) = delete;
    auto operator=(const AudioSource&) -> AudioSource& = delete;

    AudioSource(AudioSource&&) = delete;
    auto operator=(AudioSource&&) -> AudioSource& = delete;

    virtual auto start() -> bool = 0;
    virtual void stop() = 0;

    virtual auto read(std::vector<int16_t>& buffer) -> bool = 0;
};

#endif // SPEECHTOTEXT_AUDIOSOURCE_HPP
