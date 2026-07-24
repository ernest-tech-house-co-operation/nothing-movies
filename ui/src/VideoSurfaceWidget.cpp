#include "ui/VideoSurfaceWidget.h"

#include <QOpenGLContext>
#include <QMetaObject>

namespace ui {

VideoSurfaceWidget::VideoSurfaceWidget(QWidget* parent)
    : QOpenGLWidget(parent) {
    // mpv issues its own draw calls inside paintGL() via player_.render();
    // don't let QOpenGLWidget clear/composite behind our back between them.
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
}

VideoSurfaceWidget::~VideoSurfaceWidget() {
    // Destroy the mpv render context while this widget's GL context is
    // still current -- it owns GL resources (textures/FBOs) that must be
    // freed on the right context.
    makeCurrent();
    // player_'s destructor runs after this scope; explicit makeCurrent()
    // here just ensures cleanup below (if any is added later) has a
    // context. mpv_render_context_free itself is called from ~PlayerModule.
    doneCurrent();
}

void VideoSurfaceWidget::initializeGL() {
    initializeOpenGLFunctions();
    glReady_ = true;

    player::GlProcResolver resolver = [](const char* name) -> void* {
        auto* ctx = QOpenGLContext::currentContext();
        return ctx ? reinterpret_cast<void*>(ctx->getProcAddress(name)) : nullptr;
    };

    // Called from an mpv-internal thread -- hop back to the GUI thread
    // before touching the widget.
    player::RedrawCallback onRedraw = [this]() {
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    };

    player_.initGL(std::move(resolver), std::move(onRedraw));
}

void VideoSurfaceWidget::paintGL() {
    if (!glReady_) return;
    // fbo=0, flipY=true: QOpenGLWidget's own default framebuffer, which is
    // Y-flipped the same way any GL default framebuffer is.
    const qreal dpr = devicePixelRatioF();
    player_.render(defaultFramebufferObject(), static_cast<int>(width() * dpr),
                    static_cast<int>(height() * dpr), /*flipY=*/true);
}

}  // namespace ui
