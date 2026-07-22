#include "player/player.h"

#include <mpv/client.h>

#include <iostream>
#include <string>

namespace player {

struct PlayerModule::Impl {
    mpv_handle* mpv = nullptr;
    PlaybackState state = PlaybackState::Idle;
};

PlayerModule::PlayerModule() : impl_(std::make_unique<Impl>()) {}

PlayerModule::~PlayerModule() {
    if (impl_->mpv) {
        mpv_terminate_destroy(impl_->mpv);
    }
}

void PlayerModule::init(int64_t windowId) {
    impl_->mpv = mpv_create();
    if (!impl_->mpv) {
        std::cerr << "[player] failed to create mpv instance" << std::endl;
        impl_->state = PlaybackState::Error;
        return;
    }

    // Embed directly into the window ui/ hands us -- no separate mpv
    // window, it renders inside our app.
    mpv_set_option(impl_->mpv, "wid", MPV_FORMAT_INT64, &windowId);

    mpv_set_option_string(impl_->mpv, "vo", "gpu");
    mpv_set_option_string(impl_->mpv, "hwdec", "auto");
    mpv_set_option_string(impl_->mpv, "keep-open", "yes"); // don't auto-close on file end

    if (mpv_initialize(impl_->mpv) < 0) {
        std::cerr << "[player] mpv_initialize failed" << std::endl;
        impl_->state = PlaybackState::Error;
        return;
    }

    impl_->state = PlaybackState::Idle;
    std::cout << "[player] initialized" << std::endl;
}

void PlayerModule::load(const std::string& path) {
    if (!impl_->mpv) return;
    const char* cmd[] = {"loadfile", path.c_str(), nullptr};
    mpv_command(impl_->mpv, cmd);
    impl_->state = PlaybackState::Loading;
}

void PlayerModule::play() {
    if (!impl_->mpv) return;
    int flag = 0;
    mpv_set_property(impl_->mpv, "pause", MPV_FORMAT_FLAG, &flag);
    impl_->state = PlaybackState::Playing;
}

void PlayerModule::pause() {
    if (!impl_->mpv) return;
    int flag = 1;
    mpv_set_property(impl_->mpv, "pause", MPV_FORMAT_FLAG, &flag);
    impl_->state = PlaybackState::Paused;
}

void PlayerModule::stop() {
    if (!impl_->mpv) return;
    const char* cmd[] = {"stop", nullptr};
    mpv_command(impl_->mpv, cmd);
    impl_->state = PlaybackState::Stopped;
}

void PlayerModule::seek(double seconds) {
    if (!impl_->mpv) return;
    const std::string secStr = std::to_string(seconds);
    const char* cmd[] = {"seek", secStr.c_str(), "absolute", nullptr};
    mpv_command(impl_->mpv, cmd);
}

void PlayerModule::setVolume(double volume) {
    if (!impl_->mpv) return;
    mpv_set_property(impl_->mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
}

void PlayerModule::update() {
    if (!impl_->mpv) return;

    // Drain pending events without blocking (timeout 0) -- this is what
    // keeps getStatus() honest between UI polls.
    while (true) {
        mpv_event* event = mpv_wait_event(impl_->mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) break;

        if (event->event_id == MPV_EVENT_END_FILE) {
            impl_->state = PlaybackState::Stopped;
        } else if (event->event_id == MPV_EVENT_FILE_LOADED) {
            impl_->state = PlaybackState::Playing;
        }
    }
}

PlaybackStatus PlayerModule::getStatus() const {
    PlaybackStatus out;
    out.state = impl_->state;
    if (!impl_->mpv) return out;

    double pos = 0.0, dur = 0.0, vol = 100.0;
    int paused = 0;
    int coreIdle = 0;

    if (mpv_get_property(impl_->mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos) >= 0)
        out.positionSeconds = pos;
    if (mpv_get_property(impl_->mpv, "duration", MPV_FORMAT_DOUBLE, &dur) >= 0)
        out.durationSeconds = dur;
    if (mpv_get_property(impl_->mpv, "volume", MPV_FORMAT_DOUBLE, &vol) >= 0)
        out.volume = vol;
    if (mpv_get_property(impl_->mpv, "pause", MPV_FORMAT_FLAG, &paused) >= 0 && paused)
        out.state = PlaybackState::Paused;
    if (mpv_get_property(impl_->mpv, "core-idle", MPV_FORMAT_FLAG, &coreIdle) >= 0)
        out.buffering = (coreIdle != 0) && out.state == PlaybackState::Playing;

    return out;
}

} // namespace player