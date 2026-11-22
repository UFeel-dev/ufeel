install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/UFeel.Core/"
    DESTINATION adapters/csharp/UFeel.Core
    PATTERN "bin" EXCLUDE
    PATTERN "obj" EXCLUDE
    PATTERN "*.csproj" EXCLUDE
)
