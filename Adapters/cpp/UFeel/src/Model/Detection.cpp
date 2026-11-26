#include "ufeel/Model/Detection.hpp"

#include <cstddef>
#include <stdexcept>

namespace ufeel
{

auto detectionIndex(
    DetectionType feature
) -> std::size_t
{
    for (std::size_t index = 0; index < singleDetectionTypes.size(); ++index)
    {
        if (singleDetectionTypes.at(index) == feature)
        {
            return index;
        }
    }

    throw std::invalid_argument("Expected a single detection type.");
}

} // namespace ufeel
