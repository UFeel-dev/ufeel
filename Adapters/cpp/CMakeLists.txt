add_subdirectory(UFeel)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/UFeel/"
    DESTINATION adapters/cpp/UFeel
)
