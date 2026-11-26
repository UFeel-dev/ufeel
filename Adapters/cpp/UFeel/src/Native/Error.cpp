#include "ufeel/Native/Error.hpp"

#include "ufeel/Model/Status.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

namespace ufeel
{

namespace
{

auto message(
    Status status, std::string_view operation, std::string_view reason
) -> std::string
{
    return std::string("Failed to ")
        .append(operation)
        .append(": ")
        .append(reason)
        .append(" (status ")
        .append(std::to_string(static_cast<int>(status)))
        .append(").");
}

} // namespace

Error::Error(
    Status status, std::string_view operation
)
    : Error(status, operation, describe(status))
{
}

Error::Error(
    Status status, std::string_view operation, std::string_view reason
)
    : std::runtime_error(message(status, operation, reason))
    , status_(status)
{
}

auto Error::status() const noexcept -> Status
{
    return status_;
}

} // namespace ufeel
