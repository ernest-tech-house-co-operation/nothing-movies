qt_add_resources(UI_RESOURCES resources.qrc)

add_library(ui STATIC
src/main_window.cpp
src/TmdbBridge.cpp
src/SearchBridge.cpp
src/PlayerPageWidget.cpp
src/VideoSurfaceWidget.cpp
src/QueueBridge.cpp
include/ui/MainWindow.h
include/ui/TmdbBridge.h
include/ui/SearchBridge.h
include/ui/AppController.h
include/ui/PlayerPageWidget.h
include/ui/VideoSurfaceWidget.h
include/ui/QueueBridge.h
${UI_RESOURCES}
)
set_target_properties(ui PROPERTIES
AUTOMOC ON
AUTORCC ON
)
target_include_directories(ui PUBLIC include)
# OpenGLWidgets: VideoSurfaceWidget (QOpenGLWidget) is what mpv's render
# API now paints into, replacing the old createWindowContainer/wid setup.
target_link_libraries(ui PUBLIC Qt6::Widgets Qt6::OpenGLWidgets Qt6::Quick Qt6::Qml tmdb_client search_aggregator player queue_manager)