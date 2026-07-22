find_package(LibtorrentRasterbar REQUIRED)

add_library(torrent_service STATIC
    src/torrent_service.cpp
)
target_include_directories(torrent_service PUBLIC include)
target_link_libraries(torrent_service PUBLIC LibtorrentRasterbar::torrent-rasterbar)