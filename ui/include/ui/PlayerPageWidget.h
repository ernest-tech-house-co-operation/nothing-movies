#pragma once
#include <QWidget>
#include <QString>
#include <QElapsedTimer>
#include <memory>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;
class QEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QResizeEvent;

namespace queue_manager {
class Queue_managerModule;
}

namespace ui {
class VideoSurfaceWidget;
}

namespace player {
class PlayerModule;
}

namespace ui {

class PlayerPageWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayerPageWidget(std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                              QWidget* parent = nullptr);
    ~PlayerPageWidget() override;

    void setStatus(const QString& text);
    void startStream(const QString& title, const QString& url);
    void stopStream();

signals:
    void backRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void onTick();
    void onPlayPauseClicked();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    void onVolumeChanged(int value);
    void onBrightnessChanged(int value);
    void onDownloadClicked();
    void onFullscreenClicked();

private:
    void togglePlayPause();
    void skipSeconds(double seconds);
    void adjustVolume(double delta);
    void adjustBrightness(double delta);
    void toggleFullscreen();
    void beginPlayback(const std::string& path);
    void updatePlayPauseIcon(bool isPlaying);
    static QString formatTime(double seconds);
    bool isRemoteUrl(const QString& url);
    player::PlayerModule& player();
    void showOverlays();
    void armIdleHideTimer();
    void toggleHelpOverlay();                 // <-- added

    // --- Core ---
    std::shared_ptr<queue_manager::Queue_managerModule> queueManager_;
    VideoSurfaceWidget* videoWidget_ = nullptr;

    // --- Torrent/stream state ---
    QString currentStreamTitle_;
    QString currentStreamUrl_;
    QString pendingQueueId_;
    bool waitingOnQueue_ = false;
    bool userScrubbing_ = false;
    bool wasFullscreen_ = false;

    // --- Top bar ---
    QWidget* topBarWidget_ = nullptr;
    QPushButton* backButton_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    // --- Info overlay (Layer B) ---
    QWidget* infoOverlay_ = nullptr;
    QLabel* infoTitleLabel_ = nullptr;
    QLabel* infoPathLabel_ = nullptr;
    QLabel* bufferingLabel_ = nullptr;

    // --- Controls overlay (Layer C) ---
    QWidget* controlsOverlay_ = nullptr;
    QPushButton* playPauseButton_ = nullptr;
    QLabel* elapsedLabel_ = nullptr;
    QLabel* durationLabel_ = nullptr;
    QSlider* seekSlider_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
    QSlider* brightnessSlider_ = nullptr;
    QPushButton* downloadButton_ = nullptr;
    QPushButton* fullscreenButton_ = nullptr;
    QPushButton* helpButton_ = nullptr;          // <-- added

    // --- Help overlay ---
    QWidget* helpOverlay_ = nullptr;             // <-- added
    bool helpVisible_ = false;                   // <-- added

    // --- Timers ---
    QTimer* timer_ = nullptr;
    QTimer* idleHideTimer_ = nullptr;

    // --- Cinema mode ---
    QElapsedTimer lastRightArrowPress_;
    bool overlaysVisible_ = true;
    bool controlsEnabled_ = true;
};

}  // namespace ui