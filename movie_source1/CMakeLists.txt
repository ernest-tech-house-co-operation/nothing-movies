find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(movie_source1 STATIC src/movie_source1.cpp)
target_include_directories(movie_source1 PUBLIC include)
target_link_libraries(movie_source1 PUBLIC core CURL::libcurl nlohmann_json::nlohmann_json)