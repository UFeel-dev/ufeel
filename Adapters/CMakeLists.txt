set(UFEEL_ADAPTERS "" CACHE STRING "Adapters to package, semicolon-separated (e.g. csharp)")

file(GLOB adapter_lists RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*/CMakeLists.txt")
list(TRANSFORM adapter_lists REPLACE "/CMakeLists.txt$" "" OUTPUT_VARIABLE available_adapters)

foreach(adapter IN LISTS UFEEL_ADAPTERS)
    if(NOT adapter IN_LIST available_adapters)
        message(FATAL_ERROR "Unknown adapter '${adapter}' (available: ${available_adapters})")
    endif()
    add_subdirectory("${adapter}")
endforeach()
