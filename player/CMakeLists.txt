# libmpv doesn't ship a CMake config package upstream (unlike CURL or
# LibtorrentRasterbar) -- pkg-config is the standard way to find it, and
# it's what libmpv-dev installs on apt-based systems.
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED mpv)

add_library(player STATIC
    src/player.cpp
)
target_include_directories(player PUBLIC include PRIVATE ${MPV_INCLUDE_DIRS})
target_link_libraries(player PUBLIC ${MPV_LIBRARIES})