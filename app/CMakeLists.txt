add_executable(nothingmovies main.cpp)
target_link_libraries(nothingmovies PRIVATE
    core
    player
    torrent_service
    downloader
    metadata_cache
    search_aggregator
    queue_manager
    movie_source1
    movie_source2
    scraper_core
    vendor_updater
    tmdb_client
    ui
    Qt6::Widgets
)