#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>

namespace ufeel::internal
{

inline auto duplicateUtf8(
    const std::string& value, void* (*mallocFunction)(std::size_t) = std::malloc
) -> char*
{
    if (mallocFunction == nullptr)
    {
        return nullptr;
    }

    auto* result = static_cast<char*>(mallocFunction(value.size() + 1));

    if (result == nullptr)
    {
        return nullptr;
    }

    std::memcpy(result, value.c_str(), value.size() + 1);

    return result;
}

} // namespace ufeel::internal

#endif // MEMORY_HPP
