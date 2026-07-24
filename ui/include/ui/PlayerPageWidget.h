#pragma once
#include <QWidget>
#include <memory>
#include <string>

#include "queue_manager/queue_manager.h"

namespace player { class PlayerModule; }

class QLabel;
class QTimer;
class QPushButton;
class QSlider;
class QEvent;
class QPoint;

namespace ui {

class VideoSurfaceWidget;

// Page 1 of MainWindow's QStackedWidget. Holds a VideoSurfaceWidget -- a
// plain QOpenGLWidget, not a foreign native window -- that mpv paints into
// via its render API. Because it's an ordinary widget, MainWindow doesn't
// need any manual window-stacking logic to keep it correctly composited
// against page 0's QML view or the overlay controls.
//
// Flow: startStream(title, url). A magnet gets queued via queue_manager
// and polled every ~500ms until queue_manager reports readyToPlay (enough
// buffered near the front of the file for mpv to open it); a plain
// HTTP(S) url skips the queue and loads straight into mpv.
class PlayerPageWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayerPageWidget(std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                               QWidget* parent = nullptr);
    ~PlayerPageWidget() override;

    // MainWindow needs this so it doesn't accidentally lower the video
    // widget below the shell while PiP is showing it.
    bool isInPip() const { return inPip_; }

public slots:
    void startStream(const QString& title, const QString& url);
    void stopStream();

signals:
    void backRequested();
    void pipActiveChanged(bool active);

private slots:
    void onTick();
    void onPlayPauseClicked();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    void onVolumeChanged(int value);
    void onBrightnessChanged(int value);
    void onFullscreenClicked();
    void onPipClicked();
    void onDownloadClicked();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void beginPlayback(const std::string& path);
    void setStatus(const QString& text);
    void updatePlayPauseIcon(bool isPlaying);
    void togglePlayPause();
    void enterPip();
    void exitPip();
    static QString formatTime(double seconds);
    static bool isRemoteUrl(const QString& url);

    std::shared_ptr<queue_manager::Queue_managerModule> queueManager_;

    // PlayerModule needs a live GL context (this widget's) at init and
    // render time, so VideoSurfaceWidget owns the instance; this class just
    // drives it through player().
    player::PlayerModule& player();

    VideoSurfaceWidget* videoWidget_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QPushButton* backButton_ = nullptr;
    QTimer* timer_ = nullptr;

    // Playback control bar
    QWidget* topBarWidget_ = nullptr;
    QWidget* controlBarWidget_ = nullptr;
    QPushButton* playPauseButton_ = nullptr;
    QSlider* seekSlider_ = nullptr;
    QLabel* elapsedLabel_ = nullptr;
    QLabel* durationLabel_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
    QSlider* brightnessSlider_ = nullptr;
    QPushButton* fullscreenButton_ = nullptr;
    QPushButton* pipButton_ = nullptr;
    QPushButton* downloadButton_ = nullptr;

    // Picture-in-picture: a small always-on-top overlay we reparent the
    // video surface + a minimal control bar into. This is deliberately a
    // plain child widget of the top-level window (not a separate Qt::Window)
    // -- moving the native mpv render surface across genuinely separate
    // top-level windows isn't reliably supported and left the old
    // implementation with a PiP box that had working buttons but no video.
    // Keeping everything inside one top-level window sidesteps that. Not
    // OS-level PiP (that needs per-platform integration); this is an in-app
    // floating mini player so playback keeps going while you browse other
    // tabs.
    QWidget* pipWindow_ = nullptr;
    bool inPip_ = false;

    bool userScrubbing_ = false;
    bool wasFullscreen_ = false;

    bool waitingOnQueue_ = false;
    std::string pendingQueueId_;

    QString currentStreamTitle_;
    QString currentStreamUrl_;
};

}  // namespace ui
