find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

add_library(downloader STATIC
    src/downloader.cpp
)
target_include_directories(downloader PUBLIC include)
target_link_libraries(downloader PUBLIC CURL::libcurl Threads::Threads)