cmake_minimum_required(VERSION 3.20)
project(movie_source2)

# Find dependencies
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core)

# The library
add_library(movie_source2 STATIC
    src/movie_source2.cpp
)

target_include_directories(movie_source2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(movie_source2 PUBLIC
    core
    scraper_core
    CURL::libcurl
    nlohmann_json::nlohmann_json
    Qt6::Core
)

target_compile_definitions(movie_source2 PRIVATE
    MOVIE_SOURCE2_BUILD
)

# Copy data files to build directory
add_custom_command(
    TARGET movie_source2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/movie_source2/data
    COMMENT "Copying movie_source2 data files to build directory"
)