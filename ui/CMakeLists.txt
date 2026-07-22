qt_add_resources(UI_RESOURCES resources.qrc)

add_library(ui STATIC
    src/main_window.cpp
    include/ui/MainWindow.h
    ${UI_RESOURCES}
)
set_target_properties(ui PROPERTIES
    AUTOMOC ON
    AUTORCC ON
)
target_include_directories(ui PUBLIC include)
target_link_libraries(ui PUBLIC Qt6::Widgets Qt6::Quick Qt6::Qml)