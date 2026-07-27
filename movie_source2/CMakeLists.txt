find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(movie_source2 STATIC src/movie_source2.cpp)
target_include_directories(movie_source2 PUBLIC include)
target_link_libraries(movie_source2 PUBLIC
    core
    scraper_core
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

add_custom_command(
    TARGET movie_source2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/movie_source2/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        ${CMAKE_BINARY_DIR}/movie_source2/data
    COMMENT "Copying movie_source2 data files to build directory"
)