#ifndef INTERNAL_ERRORHANDLING_HPP
#define INTERNAL_ERRORHANDLING_HPP

#include <exception>
#include <iostream>
#include <utility>

namespace ufeel::internal
{

template <typename Result, typename Function>
auto guarded(
    const char* operation, Result fallback, Function&& function
) noexcept -> Result
{
    try
    {
        return std::forward<Function>(function)();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "UFeel: " << operation << " failed: " << exception.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "UFeel: " << operation << " failed with an unknown exception\n";
    }

    return fallback;
}

template <typename Function>
void guardedVoid(
    const char* operation, Function&& function
) noexcept
{
    try
    {
        std::forward<Function>(function)();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "UFeel: " << operation << " failed: " << exception.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "UFeel: " << operation << " failed with an unknown exception\n";
    }
}

} // namespace ufeel::internal

#endif // INTERNAL_ERRORHANDLING_HPP
