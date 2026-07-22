add_library(vendor_updater STATIC src/vendor_updater.cpp)
target_include_directories(vendor_updater PUBLIC include)
target_link_libraries(vendor_updater PUBLIC CURL::libcurl nlohmann_json::nlohmann_json libzip::zip)