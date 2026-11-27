#include "TestHarness/Harness.hpp"

#include <exception>
#include <iostream>

auto main() -> int
{
    try
    {
        Harness harness{UFEEL_PACKAGE_DIR};
        harness.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << "UFeel test harness: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
