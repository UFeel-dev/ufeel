#ifndef SPEECHTOTEXT_AUDIOSOURCE_HPP
#define SPEECHTOTEXT_AUDIOSOURCE_HPP

#include <cstdint>
#include <string>
#include <vector>

struct Microphone
{
    std::string name;
    bool isDefault = false;
};

class AudioSource
{
public:

    static constexpr double defaultSampleRate = 16000.0;

    AudioSource() = default;
    virtual ~AudioSource() = default;

    AudioSource(const AudioSource&) = delete;
    auto operator=(const AudioSource&) -> AudioSource& = delete;

    AudioSource(AudioSource&&) = delete;
    auto operator=(AudioSource&&) -> AudioSource& = delete;

    virtual auto start() -> bool = 0;
    virtual void stop() = 0;

    virtual auto read(std::vector<int16_t>& buffer) -> bool = 0;

    /** Mono samples per second delivered by read(), valid after a successful start(). */
    [[nodiscard]] virtual auto sampleRate() const -> double
    {
        return defaultSampleRate;
    }

    [[nodiscard]] virtual auto microphones() -> std::vector<Microphone>
    {
        return {};
    }

    /** Applies from the next start(). An empty name selects the default microphone. */
    virtual auto selectMicrophone(
        const std::string& name
    ) -> bool
    {
        return name.empty();
    }
};

#endif // SPEECHTOTEXT_AUDIOSOURCE_HPP
