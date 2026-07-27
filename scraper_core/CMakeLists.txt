find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(scraper_core STATIC
    src/scraper_core.cpp
    include/scraper_core/ScraperEngine.h
)

target_include_directories(scraper_core PUBLIC include)
target_link_libraries(scraper_core PUBLIC vendor_updater Qt6::Core)

# Ship the Node runner alongside the app binary
add_custom_command(
    TARGET scraper_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/scripts
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/piggy_runner.js
        ${CMAKE_BINARY_DIR}/scripts/piggy_runner.js
    COMMENT "Copying piggy_runner.js to build directory"
)