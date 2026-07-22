add_library(queue_manager STATIC
    src/queue_manager.cpp
)
target_include_directories(queue_manager PUBLIC include)
target_link_libraries(queue_manager PUBLIC torrent_service downloader)