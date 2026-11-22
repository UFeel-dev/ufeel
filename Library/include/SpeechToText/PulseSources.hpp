#ifndef SPEECHTOTEXT_PULSESOURCES_HPP
#define SPEECHTOTEXT_PULSESOURCES_HPP

#include <atomic>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct pa_context;
struct pa_threaded_mainloop;

struct PulseSource
{
    std::string name;
    std::string description;
    bool isMonitor = false;
};

struct PulseSources
{
    std::string defaultSource;
    std::vector<PulseSource> sources;
};

auto readPulseSources() -> std::optional<PulseSources>;

auto watchPulseSources() -> std::function<bool()>;

class PulseSourceMonitor final
{
public:

    PulseSourceMonitor();
    ~PulseSourceMonitor();

    PulseSourceMonitor(const PulseSourceMonitor&) = delete;
    auto operator=(const PulseSourceMonitor&) -> PulseSourceMonitor& = delete;

    PulseSourceMonitor(PulseSourceMonitor&&) = delete;
    auto operator=(PulseSourceMonitor&&) -> PulseSourceMonitor& = delete;

    [[nodiscard]] auto takeChanged() -> bool;

private:

    pa_threaded_mainloop* mainloop_ = nullptr;
    pa_context* context_ = nullptr;
    std::atomic<bool> changed_{false};
    std::optional<std::string> defaultSource_;

    void subscribe();
    void requestDefaultSource();
    void setDefaultSource(std::string name);
};

#endif // SPEECHTOTEXT_PULSESOURCES_HPP
