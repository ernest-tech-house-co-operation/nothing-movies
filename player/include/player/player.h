#pragma once
#include <string>
#include <memory>
#include <cstdint>
#include <functional>

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

// Resolves an OpenGL function pointer by name using the caller's current GL
// context (e.g. QOpenGLContext::currentContext()->getProcAddress(name)).
using GlProcResolver = std::function<void*(const char*)>;

// Fired from an mpv-internal thread whenever a new frame is ready to draw.
// Must only schedule a repaint (e.g. QMetaObject::invokeMethod(widget,
// "update", Qt::QueuedConnection)) -- never touch PlayerModule from here.
using RedrawCallback = std::function<void()>;

// Thin wrapper around libmpv's render API. Doesn't know about torrents,
// HTTP, or Qt -- it just plays whatever path it's given and paints itself
// into an OpenGL FBO the caller owns. That path can be:
//   - an already-finished download (queue_manager's filePath)
//   - a torrent_service filePath mid-download (mpv follows a growing file
//     fine as long as torrent_service is using sequential_download, which
//     it already is)
//   - a plain HTTP(S) URL for direct-link streaming, no download step at all
//
// This deliberately does NOT use the "wid" native-window-embedding option.
// wid hands mpv a raw window handle and makes it a foreign window that Qt
// has to fight for stacking/overlay/PiP -- exactly the bug class VLC moved
// away from years ago in favor of rendering into its own draw pipeline as
// a texture. initGL()/render() is that same approach: mpv draws into an
// FBO the UI widget already owns, so it behaves like any other paintable
// widget content -- overlays, reparenting, and PiP all "just work" because
// there's no second native window to reconcile.
class PlayerModule {
public:
    PlayerModule();
    ~PlayerModule();

    PlayerModule(const PlayerModule&) = delete;
    PlayerModule& operator=(const PlayerModule&) = delete;

    // Call once with a current GL context bound (e.g. from
    // QOpenGLWidget::initializeGL()).
    void initGL(GlProcResolver resolver, RedrawCallback onRedraw);

    void load(const std::string& path);
    void play();
    void pause();
    void stop();
    void seek(double seconds);
    void setVolume(double volume); // 0-100
    void setBrightness(double brightness); // -100 to 100, 0 = normal

    // Renders the current frame into the caller's bound FBO. Call from
    // paintGL() with a current GL context. fbo=0 targets the widget's
    // default framebuffer -- pass flipY=true in that case (GL's default
    // framebuffer is Y-flipped relative to a real FBO).
    void render(unsigned int fbo, int width, int height, bool flipY = true);

    // Pumps libmpv's event queue. Call periodically (UI timer) -- state
    // and position won't update without it.
    void update();

    PlaybackStatus getStatus() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Impl is private, so these need to be members (not free functions) to
    // be allowed to touch it from player.cpp.
    static void* glGetProcAddressTrampoline(void* ctx, const char* name);
    static void renderUpdateTrampoline(void* ctx);
};

} // namespace player
