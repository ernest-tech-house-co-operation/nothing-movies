#include "player/player.h"

#include <mpv/client.h>
#include <mpv/render_gl.h>

#include <clocale>
#include <iostream>
#include <string>

namespace player {

struct PlayerModule::Impl {
    mpv_handle* mpv = nullptr;
    mpv_render_context* renderCtx = nullptr;
    PlaybackState state = PlaybackState::Idle;
    GlProcResolver resolver;
    RedrawCallback onRedraw;
};

// mpv calls these from its own thread(s) via the ctx pointer we register --
// trampoline back into the std::function the caller gave us. Defined as
// PlayerModule members (not free functions) because Impl is private.
void* PlayerModule::glGetProcAddressTrampoline(void* ctx, const char* name) {
    auto* impl = static_cast<Impl*>(ctx);
    return impl->resolver ? impl->resolver(name) : nullptr;
}

void PlayerModule::renderUpdateTrampoline(void* ctx) {
    auto* impl = static_cast<Impl*>(ctx);
    if (impl->onRedraw) impl->onRedraw();
}

PlayerModule::PlayerModule() : impl_(std::make_unique<Impl>()) {}

PlayerModule::~PlayerModule() {
    if (impl_->renderCtx) {
        mpv_render_context_free(impl_->renderCtx);
    }
    if (impl_->mpv) {
        mpv_terminate_destroy(impl_->mpv);
    }
}

void PlayerModule::initGL(GlProcResolver resolver, RedrawCallback onRedraw) {
    // libmpv parses/formats numbers assuming the C locale (e.g. "1.5" not
    // "1,5"). If the process locale uses a different decimal separator,
    // mpv_create() fails outright -- this must run before it.
    std::setlocale(LC_NUMERIC, "C");

    impl_->resolver = std::move(resolver);
    impl_->onRedraw = std::move(onRedraw);

    impl_->mpv = mpv_create();
    if (!impl_->mpv) {
        std::cerr << "[player] failed to create mpv instance" << std::endl;
        impl_->state = PlaybackState::Error;
        return;
    }

    // "libmpv" vo hands frames to us via the render API below instead of
    // opening its own window -- no "wid" option anywhere in this file.
    mpv_set_option_string(impl_->mpv, "vo", "libmpv");
    mpv_set_option_string(impl_->mpv, "hwdec", "auto");
    mpv_set_option_string(impl_->mpv, "keep-open", "yes"); // don't auto-close on file end

    if (mpv_initialize(impl_->mpv) < 0) {
        std::cerr << "[player] mpv_initialize failed" << std::endl;
        impl_->state = PlaybackState::Error;
        return;
    }

    mpv_opengl_init_params glInitParams{};
    glInitParams.get_proc_address = &PlayerModule::glGetProcAddressTrampoline;
    glInitParams.get_proc_address_ctx = impl_.get();

    int advancedControl = 1;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInitParams},
        {MPV_RENDER_PARAM_ADVANCED_CONTROL, &advancedControl},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&impl_->renderCtx, impl_->mpv, params) < 0) {
        std::cerr << "[player] mpv_render_context_create failed" << std::endl;
        impl_->state = PlaybackState::Error;
        return;
    }

    // Fires when a new frame is ready -- we just forward it, the UI decides
    // how/when to actually repaint (queued, on the GUI thread).
    mpv_render_context_set_update_callback(impl_->renderCtx, &PlayerModule::renderUpdateTrampoline, impl_.get());

    impl_->state = PlaybackState::Idle;
    std::cout << "[player] initialized (render API, texture-based, no wid)" << std::endl;
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

void PlayerModule::setBrightness(double brightness) {
    if (!impl_->mpv) return;
    // mpv's "brightness" video-eq property: -100..100, 0 is unmodified.
    int64_t value = static_cast<int64_t>(brightness);
    mpv_set_property(impl_->mpv, "brightness", MPV_FORMAT_INT64, &value);
}

void PlayerModule::render(unsigned int fbo, int width, int height, bool flipY) {
    if (!impl_->renderCtx) return;

    mpv_opengl_fbo mpvFbo{};
    mpvFbo.fbo = static_cast<int>(fbo);
    mpvFbo.w = width;
    mpvFbo.h = height;
    mpvFbo.internal_format = 0;

    int flip = flipY ? 1 : 0;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpvFbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    mpv_render_context_render(impl_->renderCtx, params);
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
