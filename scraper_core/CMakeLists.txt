find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets)

add_library(scraper_core STATIC
    src/scraper_core.cpp
    include/scraper_core/ScraperEngine.h
)

target_include_directories(scraper_core PUBLIC include)
target_link_libraries(scraper_core PUBLIC vendor_updater Qt6::Core Qt6::Network Qt6::WebSockets)

# No more Node runner to ship - scraper_core talks to the Nothing Browser
# daemon's WebSocket protocol directly. If you still have a build directory
# from before this change, its stale scripts/piggy_runner.js can be removed.