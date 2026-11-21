#include "SpeechToText/PulseSources.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <pulse/context.h>
#include <pulse/def.h>
#include <pulse/introspect.h>
#include <pulse/mainloop.h>
#include <pulse/operation.h>
#include <pulse/subscribe.h>
#include <pulse/thread-mainloop.h>
#include <string>
#include <utility>

namespace
{

constexpr auto kTimeout = std::chrono::seconds(2);

struct MainloopDeleter
{
    void operator()(
        pa_mainloop* mainloop
    ) const
    {
        pa_mainloop_free(mainloop);
    }
};

struct ContextDeleter
{
    void operator()(
        pa_context* context
    ) const
    {
        pa_context_disconnect(context);
        pa_context_unref(context);
    }
};

struct Query
{
    PulseSources result;
    int pending = 0;
};

template <typename Condition>
auto iterateUntil(
    pa_mainloop* mainloop, Condition condition
) -> bool
{
    const auto deadline = std::chrono::steady_clock::now() + kTimeout;

    while (!condition())
    {
        const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
            deadline - std::chrono::steady_clock::now()
        );

        if (remaining.count() <= 0 ||
            pa_mainloop_prepare(mainloop, static_cast<int>(remaining.count())) < 0 ||
            pa_mainloop_poll(mainloop) < 0 || pa_mainloop_dispatch(mainloop) < 0)
        {
            return false;
        }
    }

    return true;
}

void onServerInfo(
    pa_context* /*context*/, const pa_server_info* info, void* userData
)
{
    auto* query = static_cast<Query*>(userData);

    if (info != nullptr && info->default_source_name != nullptr)
    {
        query->result.defaultSource = info->default_source_name;
    }

    --query->pending;
}

void onSourceInfo(
    pa_context* /*context*/, const pa_source_info* info, int eol, void* userData
)
{
    auto* query = static_cast<Query*>(userData);

    if (eol != 0)
    {
        --query->pending;
        return;
    }

    if (info != nullptr && info->name != nullptr)
    {
        query->result.sources.push_back({
            .name = info->name,
            .description = info->description != nullptr ? info->description : info->name,
            .isMonitor = info->monitor_of_sink != PA_INVALID_INDEX,
        });
    }
}

} // namespace

auto readPulseSources() -> std::optional<PulseSources>
{
    const std::unique_ptr<pa_mainloop, MainloopDeleter> mainloop(pa_mainloop_new());

    if (!mainloop)
    {
        return std::nullopt;
    }

    const std::unique_ptr<pa_context, ContextDeleter> context(
        pa_context_new(pa_mainloop_get_api(mainloop.get()), "UFeel")
    );

    if (!context || pa_context_connect(context.get(), nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr) < 0)
    {
        return std::nullopt;
    }

    const auto settled = [&context] -> bool {
        const pa_context_state_t state = pa_context_get_state(context.get());
        return state == PA_CONTEXT_READY || !PA_CONTEXT_IS_GOOD(state);
    };

    if (!iterateUntil(mainloop.get(), settled) ||
        pa_context_get_state(context.get()) != PA_CONTEXT_READY)
    {
        return std::nullopt;
    }

    Query query{.pending = 2};

    pa_operation* serverInfo = pa_context_get_server_info(context.get(), onServerInfo, &query);
    pa_operation* sourceInfo = pa_context_get_source_info_list(context.get(), onSourceInfo, &query);

    const bool answered =
        serverInfo != nullptr && sourceInfo != nullptr &&
        iterateUntil(mainloop.get(), [&query] -> bool { return query.pending == 0; });

    for (pa_operation* operation : {serverInfo, sourceInfo})
    {
        if (operation == nullptr)
        {
            continue;
        }
        if (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        {
            pa_operation_cancel(operation);
        }
        pa_operation_unref(operation);
    }

    return answered ? std::optional<PulseSources>(std::move(query.result)) : std::nullopt;
}

auto watchPulseSources() -> std::function<bool()>
{
    auto monitor = std::make_shared<PulseSourceMonitor>();
    return [monitor] -> bool { return monitor->takeChanged(); };
}

PulseSourceMonitor::PulseSourceMonitor()
    : mainloop_(pa_threaded_mainloop_new())
{
    if (mainloop_ == nullptr)
    {
        return;
    }

    context_ = pa_context_new(pa_threaded_mainloop_get_api(mainloop_), "UFeel");

    if (context_ == nullptr)
    {
        return;
    }

    pa_context_set_state_callback(
        context_,
        [](pa_context* context, void* userData) -> void {
            if (pa_context_get_state(context) == PA_CONTEXT_READY)
            {
                static_cast<PulseSourceMonitor*>(userData)->subscribe();
            }
        },
        this
    );

    if (pa_context_connect(context_, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr) < 0)
    {
        return;
    }

    pa_threaded_mainloop_start(mainloop_);
}

PulseSourceMonitor::~PulseSourceMonitor()
{
    if (mainloop_ != nullptr)
    {
        pa_threaded_mainloop_stop(mainloop_);
    }

    if (context_ != nullptr)
    {
        pa_context_disconnect(context_);
        pa_context_unref(context_);
    }

    if (mainloop_ != nullptr)
    {
        pa_threaded_mainloop_free(mainloop_);
    }
}

auto PulseSourceMonitor::takeChanged() -> bool
{
    return changed_.exchange(false);
}

void PulseSourceMonitor::subscribe()
{
    pa_context_set_subscribe_callback(
        context_,
        [](pa_context* /*context*/,
           pa_subscription_event_type_t event,
           uint32_t /*index*/,
           void* userData) -> void {
            auto* monitor = static_cast<PulseSourceMonitor*>(userData);
            const auto facility = event & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
            const auto type = event & PA_SUBSCRIPTION_EVENT_TYPE_MASK;

            if (facility == PA_SUBSCRIPTION_EVENT_SOURCE && type != PA_SUBSCRIPTION_EVENT_CHANGE)
            {
                monitor->changed_ = true;
            }
            else if (facility == PA_SUBSCRIPTION_EVENT_SERVER)
            {
                monitor->requestDefaultSource();
            }
        },
        this
    );

    const auto mask = static_cast<pa_subscription_mask_t>(
        PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SERVER
    );

    if (pa_operation* operation = pa_context_subscribe(context_, mask, nullptr, nullptr))
    {
        pa_operation_unref(operation);
    }

    requestDefaultSource();
}

void PulseSourceMonitor::requestDefaultSource()
{
    pa_operation* operation = pa_context_get_server_info(
        context_,
        [](pa_context* /*context*/, const pa_server_info* info, void* userData) -> void {
            if (info != nullptr && info->default_source_name != nullptr)
            {
                static_cast<PulseSourceMonitor*>(userData)->setDefaultSource(
                    info->default_source_name
                );
            }
        },
        this
    );

    if (operation != nullptr)
    {
        pa_operation_unref(operation);
    }
}

void PulseSourceMonitor::setDefaultSource(
    std::string name
)
{
    if (defaultSource_.has_value() && *defaultSource_ != name)
    {
        changed_ = true;
    }

    defaultSource_ = std::move(name);
}
