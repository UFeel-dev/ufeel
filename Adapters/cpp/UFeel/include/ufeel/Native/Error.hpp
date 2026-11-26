#ifndef UFEEL_NATIVE_ERROR_HPP
#define UFEEL_NATIVE_ERROR_HPP

#include "ufeel/Model/Status.hpp"

#include <stdexcept>
#include <string_view>

namespace ufeel
{

class Error : public std::runtime_error
{
public:

    Error(Status status, std::string_view operation);
    Error(Status status, std::string_view operation, std::string_view reason);

    [[nodiscard]] auto status() const noexcept -> Status;

private:

    Status status_;
};

} // namespace ufeel

#endif // UFEEL_NATIVE_ERROR_HPP
