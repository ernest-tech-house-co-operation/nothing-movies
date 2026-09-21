find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(movie_source3 STATIC src/movie_source3.cpp)
target_include_directories(movie_source3 PUBLIC include)
target_link_libraries(movie_source3 PUBLIC core CURL::libcurl nlohmann_json::nlohmann_json)

add_custom_command(
    TARGET movie_source3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/movie_source3/data
    COMMENT "Copying movie_source3 data files to build directory"
)