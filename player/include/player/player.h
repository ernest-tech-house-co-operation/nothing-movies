#pragma once
#include <string>
#include <memory>
#include <cstdint>

namespace player {

enum class PlaybackState {
    Idle,
    Loading,
    Playing,
    Paused,
    Stopped,
    Error
};

struct PlaybackStatus {
    PlaybackState state = PlaybackState::Idle;
    double positionSeconds = 0.0;
    double durationSeconds = 0.0;
    double volume = 100.0;   // 0-100
    bool buffering = false;
};

// Thin wrapper around libmpv. Doesn't know about torrents, HTTP, or Qt --
// it just plays whatever path it's given. That path can be:
//   - an already-finished download (queue_manager's filePath)
//   - a torrent_service filePath mid-download (mpv follows a growing file
//     fine as long as torrent_service is using sequential_download, which
//     it already is)
//   - a plain HTTP(S) URL for direct-link streaming, no download step at all
class PlayerModule {
public:
    PlayerModule();
    ~PlayerModule();

    PlayerModule(const PlayerModule&) = delete;
    PlayerModule& operator=(const PlayerModule&) = delete;

    // windowId is the native window handle (X11 Window / HWND) that mpv
    // renders video into. ui/ creates the surface and owns that handle;
    // player/ just embeds into it.
    void init(int64_t windowId);

    void load(const std::string& path);
    void play();
    void pause();
    void stop();
    void seek(double seconds);
    void setVolume(double volume); // 0-100

    // Pumps libmpv's event queue. Call periodically (UI timer) -- state
    // and position won't update without it.
    void update();

    PlaybackStatus getStatus() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace player