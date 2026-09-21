add_library(ui STATIC
    resources.qrc
    src/main_window.cpp
    src/SearchBridge.cpp
    src/HomepageBridge.cpp
    src/PlayerPageWidget.cpp
    src/StreamEmbedWidget.cpp
    src/VideoSurfaceWidget.cpp
    src/QueueBridge.cpp
    src/TorrentFileSelectDialog.cpp
    src/TorrentSelectDialog.cpp
    src/SplashWidget.cpp
    src/ShellWidget.cpp
    src/HomeWidget.cpp
    src/SearchWidget.cpp
    src/InfoWidget.cpp
    src/DownloadsWidget.cpp
    src/SourcesWidget.cpp
    src/SettingsWidget.cpp
    src/ThanksWidget.cpp
    src/ImageLoader.cpp
    include/ui/MainWindow.h
    include/ui/SearchBridge.h
    include/ui/HomepageBridge.h
    include/ui/AppController.h
    include/ui/PlayerPageWidget.h
    include/ui/StreamEmbedWidget.h
    include/ui/VideoSurfaceWidget.h
    include/ui/QueueBridge.h
    include/ui/TorrentFileSelectDialog.h
    include/ui/TorrentSelectDialog.h
    include/ui/SplashWidget.h
    include/ui/ShellWidget.h
    include/ui/HomeWidget.h
    include/ui/SearchWidget.h
    include/ui/InfoWidget.h
    include/ui/DownloadsWidget.h
    include/ui/SourcesWidget.h
    include/ui/SettingsWidget.h
    include/ui/ThanksWidget.h
    include/ui/ImageLoader.h
    include/ui/Theme.h
)

set_target_properties(ui PROPERTIES
    AUTOMOC ON
)

target_include_directories(ui PUBLIC include)

target_link_libraries(ui PUBLIC
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Network
    search_aggregator
    player
    queue_manager
)

if(NOT WIN32)
    target_link_libraries(ui PRIVATE Qt6::WebEngineWidgets)
endif()