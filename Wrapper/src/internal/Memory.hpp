#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdlib>
#include <cstring>
#include <string>

namespace ufeel::internal
{

inline auto duplicateUtf8(
    const std::string& value
) -> char*
{
    auto* result = static_cast<char*>(std::malloc(value.size() + 1));

    if (result == nullptr)
    {
        return nullptr;
    }

    std::memcpy(result, value.c_str(), value.size() + 1);

    return result;
}

} // namespace ufeel::internal

#endif // MEMORY_HPP
