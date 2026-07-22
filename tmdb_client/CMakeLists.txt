find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(tmdb_client STATIC
    src/TitleCleaner.cpp
    src/TmdbClient.cpp
)
target_include_directories(tmdb_client PUBLIC include)
target_link_libraries(tmdb_client PUBLIC CURL::libcurl nlohmann_json::nlohmann_json)