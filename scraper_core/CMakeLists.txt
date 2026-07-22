add_library(scraper_core STATIC
    src/scraper_core.cpp
)
target_include_directories(scraper_core PUBLIC include)
target_link_libraries(scraper_core PUBLIC vendor_updater)
