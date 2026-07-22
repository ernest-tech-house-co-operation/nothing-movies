add_library(search_aggregator STATIC
    src/search_aggregator.cpp
)
target_include_directories(search_aggregator PUBLIC include)
target_link_libraries(search_aggregator PUBLIC core)