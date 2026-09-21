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
    movie_source3
    ui
    Qt6::Widgets
)

install(TARGETS nothingmovies
    RUNTIME DESTINATION bin
)

install(FILES
    ${CMAKE_SOURCE_DIR}/nothingmovies.desktop
    DESTINATION share/applications
)

install(FILES
    ${CMAKE_SOURCE_DIR}/ui/resources/logo.png
    DESTINATION share/icons/hicolor/512x512/apps
    RENAME nothingmovies.png
)