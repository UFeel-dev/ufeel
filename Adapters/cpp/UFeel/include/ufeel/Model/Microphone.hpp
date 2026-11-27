#ifndef UFEEL_MODEL_MICROPHONE_HPP
#define UFEEL_MODEL_MICROPHONE_HPP

#include <string>

namespace ufeel
{

struct Microphone
{
    std::string id;
    std::string name;
    bool isDefault = false;
};

} // namespace ufeel

#endif // UFEEL_MODEL_MICROPHONE_HPP
