find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(movie_source1 STATIC src/movie_source1.cpp)
target_include_directories(movie_source1 PUBLIC include)
target_link_libraries(movie_source1 PUBLIC core CURL::libcurl nlohmann_json::nlohmann_json)

# Copy data folder (info.json) into build directory so the binary can find
# it at runtime via the relative path "movie_source1/data/info.json".
# This is the same pattern used by movie_source3's symlink — CMake handles
# it automatically on every build so you never have to copy it manually.
# Plugin guide part 1: "info.json is the single source of truth — don't lose it."
add_custom_command(
    TARGET movie_source1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/movie_source1/data
    COMMENT "Copying movie_source1 data files to build directory"
)