#ifndef ADAPTERS_CPP_TESTSUPPORT_EVENTUALLY_HPP
#define ADAPTERS_CPP_TESTSUPPORT_EVENTUALLY_HPP

#include <chrono>
#include <thread>

template <typename Predicate>
auto eventually(
    Predicate predicate, std::chrono::milliseconds timeout = std::chrono::seconds(10)
) -> bool
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (!predicate())
    {
        if (std::chrono::steady_clock::now() > deadline)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

#endif // ADAPTERS_CPP_TESTSUPPORT_EVENTUALLY_HPP
