#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include "player/player.h"

namespace ui {

// A plain widget that paints mpv video into its own paintGL(). This is the
// replacement for the old QWindow-via-createWindowContainer approach: mpv
// no longer owns a native window at all, it just draws a texture into an
// FBO this widget hands it -- same trick VLC's Qt frontend uses. Because
// it's a normal QWidget now (not a foreign surface glued in), it composites
// correctly under overlay controls, survives being reparented into the PiP
// container, and doesn't need any manual window-stacking workarounds in
// MainWindow.
class VideoSurfaceWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit VideoSurfaceWidget(QWidget* parent = nullptr);
    ~VideoSurfaceWidget() override;

    // Exposed so PlayerPageWidget can drive playback -- the widget owns the
    // PlayerModule instance because the module needs a live GL context
    // (this widget's) at both init and render time.
    player::PlayerModule& player() { return player_; }

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    player::PlayerModule player_;
    bool glReady_ = false;
};

}  // namespace ui
