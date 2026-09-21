#pragma once
#include <QWidget>
#include <QString>
#include <QElapsedTimer>
#include <memory>

class QLabel;
class QPushButton;
class QSlider;
class QTimer;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QResizeEvent;
class VideoSurfaceWidget;

namespace queue_manager { class Queue_managerModule; }

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
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onTick();
    void onPlayPauseClicked();
    void onSeekSliderPressed();
    void onSeekSliderReleased();
    void onVolumeChanged(int value);
    void onFullscreenClicked();
    void onDownloadClicked();
private:
    void beginPlayback(const QString& path);

    // UI helpers
    void togglePlayPause();
    void skipSeconds(double seconds);
    void adjustVolume(double delta);
    void toggleFullscreen();
    void toggleHelpOverlay();
    void showOverlays();
    void armIdleHideTimer();
    void updatePlayPauseIcon(bool isPlaying);
    static QString formatTime(double seconds);
    bool isRemoteUrl(const QString& url);

    std::shared_ptr<queue_manager::Queue_managerModule> queueManager_;

    VideoSurfaceWidget* surface_ = nullptr;

    // playback state (updated via IPC events)
    bool   isPlaying_   = false;
    double position_    = 0.0;
    double duration_    = 0.0;
    double volume_      = 100.0;
    bool   userScrubbing_ = false;
    bool   waitingOnQueue_ = false;
    QString pendingQueueId_;

    QString currentStreamTitle_;
    QString currentStreamUrl_;

    // window
    bool wasFullscreen_ = false;

    // overlays
    QWidget*     topBarWidget_    = nullptr;
    QPushButton* backButton_      = nullptr;
    QLabel*      titleLabel_      = nullptr;
    QLabel*      statusLabel_     = nullptr;

    QWidget*     infoOverlay_     = nullptr;
    QLabel*      infoTitleLabel_  = nullptr;
    QLabel*      infoPathLabel_   = nullptr;
    QLabel*      bufferingLabel_  = nullptr;

    QWidget*     controlsOverlay_ = nullptr;
    QPushButton* playPauseButton_ = nullptr;
    QLabel*      elapsedLabel_    = nullptr;
    QLabel*      durationLabel_   = nullptr;
    QSlider*     seekSlider_      = nullptr;
    QSlider*     volumeSlider_    = nullptr;
    QPushButton* downloadButton_  = nullptr;
    QPushButton* fullscreenButton_= nullptr;
    QPushButton* helpButton_      = nullptr;

    QWidget*     helpOverlay_     = nullptr;
    bool         helpVisible_     = false;

    QTimer*      timer_           = nullptr;
    QTimer*      idleHideTimer_   = nullptr;
    bool         overlaysVisible_ = true;
    bool         controlsEnabled_ = true;

    QElapsedTimer lastRightArrowPress_;
};