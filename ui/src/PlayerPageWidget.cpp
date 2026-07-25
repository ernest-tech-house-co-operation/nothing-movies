#include "ui/PlayerPageWidget.h"
#include "ui/VideoSurfaceWidget.h"
#include "player/player.h"
#include "queue_manager/queue_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QStyle>
#include <QGuiApplication>
#include <QScreen>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <algorithm>
#include <cmath>

namespace ui {

namespace {

// Slider that jumps to click position
class ClickToSeekSlider : public QSlider {
public:
    explicit ClickToSeekSlider(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QSlider(orientation, parent) {}

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            const int newValue = QStyle::sliderValueFromPosition(
                minimum(), maximum(), event->pos().x(), width());
            setValue(newValue);
        }
        QSlider::mousePressEvent(event);
    }
};

constexpr int kIdleHideMs = 3500;
constexpr int kDoubleTapMs = 400;
constexpr double kSeekSmallSeconds = 10.0;
constexpr double kSeekDoubleTapSeconds = 20.0;

}  // namespace

PlayerPageWidget::PlayerPageWidget(std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                                   QWidget* parent)
    : QWidget(parent), queueManager_(std::move(queueManager)) {
    setStyleSheet("background-color: #000000;");
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    videoWidget_ = new VideoSurfaceWidget(this);
    videoWidget_->setFocusPolicy(Qt::NoFocus);
    videoWidget_->installEventFilter(this);
    videoWidget_->setMouseTracking(true);
    videoWidget_->lower();

    // ============================================================
    // Layer B: Info overlay (floating green)
    // ============================================================
    infoOverlay_ = new QWidget(this);
    infoOverlay_->setAttribute(Qt::WA_TransparentForMouseEvents);
    infoOverlay_->setStyleSheet("background: transparent;");

    infoTitleLabel_ = new QLabel(infoOverlay_);
    infoTitleLabel_->setStyleSheet(
        "color: #6fae6f; font-size: 14px; font-weight: 600; background: transparent;");

    infoPathLabel_ = new QLabel(infoOverlay_);
    infoPathLabel_->setStyleSheet(
        "color: #6fae6f; font-size: 12px; background: transparent;");

    bufferingLabel_ = new QLabel(infoOverlay_);
    bufferingLabel_->setStyleSheet(
        "color: #6fae6f; font-size: 13px; background: transparent;");
    bufferingLabel_->hide();

    auto* infoLayout = new QVBoxLayout(infoOverlay_);
    infoLayout->setContentsMargins(16, 12, 16, 8);
    infoLayout->setSpacing(2);
    infoLayout->addWidget(infoTitleLabel_);
    infoLayout->addWidget(infoPathLabel_);
    infoLayout->addWidget(bufferingLabel_);
    infoLayout->addStretch(1);

    // ============================================================
    // Layer C: Bottom controls (floating)
    // ============================================================
    controlsOverlay_ = new QWidget(this);
    controlsOverlay_->setStyleSheet("background-color: rgba(0, 0, 0, 160); border-radius: 6px;");

    playPauseButton_ = new QPushButton(controlsOverlay_);
    playPauseButton_->setFixedWidth(40);
    updatePlayPauseIcon(false);
    connect(playPauseButton_, &QPushButton::clicked, this, &PlayerPageWidget::onPlayPauseClicked);

    elapsedLabel_ = new QLabel("0:00", controlsOverlay_);
    elapsedLabel_->setStyleSheet("color: #cccccc; font-size: 12px;");

    durationLabel_ = new QLabel("0:00", controlsOverlay_);
    durationLabel_->setStyleSheet("color: #cccccc; font-size: 12px;");

    seekSlider_ = new ClickToSeekSlider(Qt::Horizontal, controlsOverlay_);
    seekSlider_->setRange(0, 1000);
    seekSlider_->setValue(0);
    connect(seekSlider_, &QSlider::sliderPressed, this, &PlayerPageWidget::onSeekSliderPressed);
    connect(seekSlider_, &QSlider::sliderReleased, this, &PlayerPageWidget::onSeekSliderReleased);

    auto* volumeIcon = new QLabel("🔊", controlsOverlay_);
    volumeSlider_ = new QSlider(Qt::Horizontal, controlsOverlay_);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(100);
    volumeSlider_->setFixedWidth(90);
    connect(volumeSlider_, &QSlider::valueChanged, this, &PlayerPageWidget::onVolumeChanged);

    auto* brightnessIcon = new QLabel("🔆", controlsOverlay_);
    brightnessSlider_ = new QSlider(Qt::Horizontal, controlsOverlay_);
    brightnessSlider_->setRange(-60, 60);
    brightnessSlider_->setValue(0);
    brightnessSlider_->setFixedWidth(90);
    connect(brightnessSlider_, &QSlider::valueChanged, this, &PlayerPageWidget::onBrightnessChanged);

    downloadButton_ = new QPushButton("⬇ Download", controlsOverlay_);
    downloadButton_->setToolTip("Save this to Downloads while it plays");
    downloadButton_->setVisible(false);
    connect(downloadButton_, &QPushButton::clicked, this, &PlayerPageWidget::onDownloadClicked);

    fullscreenButton_ = new QPushButton("⛶", controlsOverlay_);
    fullscreenButton_->setFixedWidth(36);
    connect(fullscreenButton_, &QPushButton::clicked, this, &PlayerPageWidget::onFullscreenClicked);

    // --- Help button ---
    helpButton_ = new QPushButton("?", controlsOverlay_);
    helpButton_->setFixedWidth(30);
    helpButton_->setToolTip("Show keyboard shortcuts");
    connect(helpButton_, &QPushButton::clicked, this, [this]() {
        toggleHelpOverlay();
    });

    auto* seekRow = new QHBoxLayout();
    seekRow->setSpacing(10);
    seekRow->addWidget(playPauseButton_);
    seekRow->addWidget(elapsedLabel_);
    seekRow->addWidget(seekSlider_, 1);
    seekRow->addWidget(durationLabel_);

    auto* toolsRow = new QHBoxLayout();
    toolsRow->setSpacing(8);
    toolsRow->addWidget(volumeIcon);
    toolsRow->addWidget(volumeSlider_);
    toolsRow->addSpacing(12);
    toolsRow->addWidget(brightnessIcon);
    toolsRow->addWidget(brightnessSlider_);
    toolsRow->addStretch(1);
    toolsRow->addWidget(downloadButton_);
    toolsRow->addWidget(fullscreenButton_);
    toolsRow->addWidget(helpButton_);          // <-- added

    auto* controlsLayout = new QVBoxLayout(controlsOverlay_);
    controlsLayout->setContentsMargins(16, 8, 16, 10);
    controlsLayout->setSpacing(6);
    controlsLayout->addLayout(seekRow);
    controlsLayout->addLayout(toolsRow);

    // ============================================================
    // Help overlay - shows keyboard shortcuts
    // ============================================================
    helpOverlay_ = new QWidget(this);
    helpOverlay_->setStyleSheet("background-color: rgba(0, 0, 0, 200); border-radius: 8px;");
    helpOverlay_->hide();

    auto* helpLabel = new QLabel(helpOverlay_);
    helpLabel->setStyleSheet("color: #eeeeee; font-size: 13px;");
    helpLabel->setTextFormat(Qt::RichText);
    helpLabel->setText(
        "<b>Controls</b><br>"
        "Space &nbsp;&nbsp; Play / Pause<br>"
        "&larr; / &rarr; &nbsp;&nbsp; Seek -10s / +10s (double-tap &rarr; for +20s)<br>"
        "&uarr; / &darr; &nbsp;&nbsp; Volume +5 / -5<br>"
        "W / S &nbsp;&nbsp; Brightness +5 / -5<br>"
        "A / D &nbsp;&nbsp; Brightness -1 / +1 (fine)<br>"
        "F &nbsp;&nbsp; Toggle fullscreen<br>"
        "Esc &nbsp;&nbsp; Exit fullscreen<br>"
        "H / ? &nbsp;&nbsp; Toggle this help<br>"
    );

    auto* helpLayout = new QVBoxLayout(helpOverlay_);
    helpLayout->setContentsMargins(20, 16, 20, 16);
    helpLayout->addWidget(helpLabel);

    // ============================================================
    // Top bar (Back + title – for navigation)
    // ============================================================
    topBarWidget_ = new QWidget(this);
    topBarWidget_->setStyleSheet("background: rgba(0,0,0,150); border-radius: 4px;");
    backButton_ = new QPushButton("< Back", topBarWidget_);
    connect(backButton_, &QPushButton::clicked, this, [this]() {
        stopStream();
        emit backRequested();
    });
    titleLabel_ = new QLabel(topBarWidget_);
    titleLabel_->setStyleSheet("color: white; font-size: 14px;");
    statusLabel_ = new QLabel(topBarWidget_);
    statusLabel_->setStyleSheet("color: #aaaaaa; font-size: 12px;");
    statusLabel_->hide();

    auto* topLayout = new QHBoxLayout(topBarWidget_);
    topLayout->setContentsMargins(12, 6, 12, 6);
    topLayout->addWidget(backButton_);
    topLayout->addWidget(titleLabel_, 1);
    topLayout->addWidget(statusLabel_);

    // ============================================================
    // Styling
    // ============================================================
    setStyleSheet(styleSheet() +
        "QPushButton { background-color: #171225; color: white; border: 1px solid #7c3aed;"
        "              border-radius: 6px; padding: 4px 10px; }"
        "QPushButton:hover { background-color: #2a2140; }"
        "QSlider::groove:horizontal { background: #2a2a3a; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #6fae6f; width: 12px; height: 12px;"
        "                             margin: -4px 0; border-radius: 6px; }"
        "QSlider::sub-page:horizontal { background: #6fae6f; border-radius: 2px; }"
    );

    topBarWidget_->raise();
    infoOverlay_->raise();
    controlsOverlay_->raise();
    helpOverlay_->raise();          // <-- added

    // ============================================================
    // Timers
    // ============================================================
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &PlayerPageWidget::onTick);
    timer_->start(500);

    idleHideTimer_ = new QTimer(this);
    idleHideTimer_->setSingleShot(true);
    connect(idleHideTimer_, &QTimer::timeout, this, [this]() {
        if (overlaysVisible_ && controlsEnabled_) {
            overlaysVisible_ = false;
            topBarWidget_->hide();
            infoOverlay_->hide();
            controlsOverlay_->hide();
            setCursor(Qt::BlankCursor);
        }
    });

    overlaysVisible_ = true;
    controlsEnabled_ = true;
    armIdleHideTimer();
}

PlayerPageWidget::~PlayerPageWidget() = default;

player::PlayerModule& PlayerPageWidget::player() {
    return videoWidget_->player();
}

void PlayerPageWidget::setStatus(const QString& text) {
    statusLabel_->setText(text);
    statusLabel_->setVisible(!text.isEmpty());
    if (text.isEmpty()) {
        bufferingLabel_->hide();
    } else {
        bufferingLabel_->setText(text);
        bufferingLabel_->show();
    }
}

void PlayerPageWidget::startStream(const QString& title, const QString& url) {
    titleLabel_->setText(title);
    infoTitleLabel_->setText(title);
    infoPathLabel_->setText(url);

    waitingOnQueue_ = false;
    pendingQueueId_.clear();

    currentStreamTitle_ = title;
    currentStreamUrl_ = url;
    downloadButton_->setEnabled(true);
    downloadButton_->setText("⬇ Download");
    downloadButton_->setVisible(isRemoteUrl(url));

    const std::string urlStd = url.toStdString();
    if (urlStd.rfind("magnet:", 0) == 0) {
        setStatus("Queuing torrent...");
        pendingQueueId_ = QString::fromStdString(
            queueManager_->enqueueTorrent(title.toStdString(), urlStd));
        if (pendingQueueId_.isEmpty()) {
            setStatus("Failed to queue torrent.");
            return;
        }
        waitingOnQueue_ = true;
    } else {
        beginPlayback(urlStd);
    }

    showOverlays();
}

void PlayerPageWidget::beginPlayback(const std::string& path) {
    setStatus("Loading...");
    player().load(path);
    player().play();
    showOverlays();
}

void PlayerPageWidget::stopStream() {
    waitingOnQueue_ = false;
    pendingQueueId_.clear();
    player().stop();
    setStatus(QString());
}

void PlayerPageWidget::onTick() {
    queueManager_->update();
    player().update();

    if (waitingOnQueue_ && !pendingQueueId_.isEmpty()) {
        const auto item = queueManager_->getItem(pendingQueueId_.toStdString());
        if (item.state == queue_manager::QueueItemState::Error) {
            waitingOnQueue_ = false;
            setStatus("Torrent failed to start.");
        } else if (item.readyToPlay && !item.filePath.empty()) {
            waitingOnQueue_ = false;
            beginPlayback(item.filePath);
        } else {
            const int pct = static_cast<int>(item.progress * 100.0);
            setStatus(QString("Buffering... %1%").arg(pct));
        }
        return;
    }

    const auto status = player().getStatus();
    if (status.state == player::PlaybackState::Playing && !status.buffering) {
        setStatus(QString());
    } else if (status.buffering) {
        setStatus("Buffering...");
    } else if (status.state == player::PlaybackState::Error) {
        setStatus("Playback error.");
    }

    updatePlayPauseIcon(status.state == player::PlaybackState::Playing);

    if (!userScrubbing_) {
        if (status.durationSeconds > 0.0) {
            const int sliderPos = static_cast<int>(
                (status.positionSeconds / status.durationSeconds) * 1000.0);
            seekSlider_->setValue(sliderPos);
        } else {
            seekSlider_->setValue(0);
        }
    }

    elapsedLabel_->setText(formatTime(status.positionSeconds));
    durationLabel_->setText(formatTime(status.durationSeconds));
}

void PlayerPageWidget::togglePlayPause() {
    const auto status = player().getStatus();
    if (status.state == player::PlaybackState::Playing) {
        player().pause();
    } else {
        player().play();
    }
    showOverlays();
    armIdleHideTimer();
}

void PlayerPageWidget::onPlayPauseClicked() {
    togglePlayPause();
}

void PlayerPageWidget::onSeekSliderPressed() {
    userScrubbing_ = true;
}

void PlayerPageWidget::onSeekSliderReleased() {
    const auto status = player().getStatus();
    if (status.durationSeconds > 0.0) {
        const double target = (seekSlider_->value() / 1000.0) * status.durationSeconds;
        player().seek(target);
    }
    userScrubbing_ = false;
    showOverlays();
    armIdleHideTimer();
}

void PlayerPageWidget::onVolumeChanged(int value) {
    player().setVolume(static_cast<double>(value));
    showOverlays();
    armIdleHideTimer();
}

void PlayerPageWidget::onBrightnessChanged(int value) {
    player().setBrightness(static_cast<double>(value));
    showOverlays();
    armIdleHideTimer();
}

void PlayerPageWidget::onDownloadClicked() {
    if (currentStreamUrl_.isEmpty() || !isRemoteUrl(currentStreamUrl_)) return;

    const std::string id = queueManager_->enqueueHttpDownload(
        currentStreamTitle_.toStdString(), currentStreamUrl_.toStdString());

    if (!id.empty()) {
        setStatus("Added to Downloads.");
        downloadButton_->setText("✓ Downloading");
        downloadButton_->setEnabled(false);
    } else {
        setStatus("Couldn't start download.");
    }
}

bool PlayerPageWidget::isRemoteUrl(const QString& url) {
    return url.startsWith("http://", Qt::CaseInsensitive) ||
           url.startsWith("https://", Qt::CaseInsensitive);
}

// ============================================================
// Keyboard shortcuts
// ============================================================
void PlayerPageWidget::keyPressEvent(QKeyEvent* event) {
    showOverlays();
    armIdleHideTimer();

    switch (event->key()) {
        case Qt::Key_Space:
            togglePlayPause();
            break;

        case Qt::Key_Right: {
            if (lastRightArrowPress_.isValid() && lastRightArrowPress_.elapsed() < kDoubleTapMs) {
                skipSeconds(kSeekDoubleTapSeconds);
            } else {
                skipSeconds(kSeekSmallSeconds);
            }
            lastRightArrowPress_.restart();
            break;
        }

        case Qt::Key_Left:
            skipSeconds(-kSeekSmallSeconds);
            break;

        case Qt::Key_Up:
            adjustVolume(5.0);
            break;
        case Qt::Key_Down:
            adjustVolume(-5.0);
            break;

        case Qt::Key_W:
            adjustBrightness(5.0);
            break;
        case Qt::Key_S:
            adjustBrightness(-5.0);
            break;
        case Qt::Key_D:
            adjustBrightness(1.0);
            break;
        case Qt::Key_A:
            adjustBrightness(-1.0);
            break;

        case Qt::Key_F:
            toggleFullscreen();
            break;
        case Qt::Key_Escape:
            if (wasFullscreen_) toggleFullscreen();
            break;

        case Qt::Key_H:
        case Qt::Key_Question:          // '?' key
            toggleHelpOverlay();
            break;

        default:
            QWidget::keyPressEvent(event);
            return;
    }
    event->accept();
}

void PlayerPageWidget::skipSeconds(double seconds) {
    const auto status = player().getStatus();
    double target = status.positionSeconds + seconds;
    if (target < 0.0) target = 0.0;
    if (status.durationSeconds > 0.0) target = std::min(target, status.durationSeconds);
    player().seek(target);
}

void PlayerPageWidget::adjustVolume(double delta) {
    const int newValue = std::clamp(volumeSlider_->value() + static_cast<int>(delta), 0, 100);
    volumeSlider_->setValue(newValue);
    bufferingLabel_->setText(QString("🔊 %1%").arg(newValue));
    bufferingLabel_->show();
    QTimer::singleShot(1500, [this]() {
        if (bufferingLabel_->text().startsWith("🔊")) {
            bufferingLabel_->hide();
        }
    });
}

void PlayerPageWidget::adjustBrightness(double delta) {
    const int newValue = std::clamp(brightnessSlider_->value() + static_cast<int>(delta), -60, 60);
    brightnessSlider_->setValue(newValue);
    bufferingLabel_->setText(QString("☀ %1%").arg(newValue));
    bufferingLabel_->show();
    QTimer::singleShot(1500, [this]() {
        if (bufferingLabel_->text().startsWith("☀")) {
            bufferingLabel_->hide();
        }
    });
}

// ============================================================
// Fullscreen
// ============================================================
void PlayerPageWidget::onFullscreenClicked() {
    toggleFullscreen();
}

void PlayerPageWidget::toggleFullscreen() {
    auto* win = window();
    if (!win) return;

    if (!wasFullscreen_) {
        win->showFullScreen();
        fullscreenButton_->setText("🗗");
    } else {
        win->showNormal();
        fullscreenButton_->setText("⛶");
    }
    wasFullscreen_ = !wasFullscreen_;
}

// ============================================================
// Mouse events (cinema mode)
// ============================================================
bool PlayerPageWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == videoWidget_) {
        if (event->type() == QEvent::MouseMove) {
            showOverlays();
            armIdleHideTimer();
        } else if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                togglePlayPause();
                showOverlays();
                armIdleHideTimer();
                return true;
            }
        } else if (event->type() == QEvent::Wheel) {
            auto* wheelEvent = static_cast<QWheelEvent*>(event);
            const int steps = wheelEvent->angleDelta().y() / 120;
            if (steps != 0) {
                adjustVolume(steps * 5.0);
            }
            showOverlays();
            armIdleHideTimer();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PlayerPageWidget::mouseMoveEvent(QMouseEvent* event) {
    showOverlays();
    armIdleHideTimer();
    QWidget::mouseMoveEvent(event);
}

// ============================================================
// Overlay management (cinema mode)
// ============================================================
void PlayerPageWidget::showOverlays() {
    overlaysVisible_ = true;
    topBarWidget_->show();
    infoOverlay_->show();
    controlsOverlay_->show();
    unsetCursor();
}

void PlayerPageWidget::armIdleHideTimer() {
    // If help is visible, keep overlays up indefinitely (don't auto‑hide)
    if (!helpVisible_) {
        idleHideTimer_->start(kIdleHideMs);
    }
}

// ============================================================
// Help overlay toggle
// ============================================================
void PlayerPageWidget::toggleHelpOverlay() {
    helpVisible_ = !helpVisible_;
    helpOverlay_->setVisible(helpVisible_);
    if (helpVisible_) {
        helpOverlay_->raise();
        // Stop auto‑hide while help is shown
        idleHideTimer_->stop();
        // Also show the other overlays so the user can see the context
        showOverlays();
    } else {
        // Re‑arm the idle timer when help is dismissed
        armIdleHideTimer();
    }
}

// ============================================================
// Resize
// ============================================================
void PlayerPageWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    videoWidget_->setGeometry(rect());

    topBarWidget_->setGeometry(10, 10, width() - 20, 44);

    infoOverlay_->setGeometry(10, 60, width() - 20, 100);

    const int controlsHeight = 100;
    controlsOverlay_->setGeometry(10, height() - controlsHeight - 10, width() - 20, controlsHeight);

    // Help overlay: centered
    const int helpWidth = 340;
    const int helpHeight = 220;
    helpOverlay_->setGeometry(
        (width() - helpWidth) / 2, (height() - helpHeight) / 2, helpWidth, helpHeight);
}

// ============================================================
// Helpers
// ============================================================
void PlayerPageWidget::updatePlayPauseIcon(bool isPlaying) {
    playPauseButton_->setText(isPlaying ? "⏸" : "▶");
}

QString PlayerPageWidget::formatTime(double seconds) {
    if (seconds < 0.0 || std::isnan(seconds)) seconds = 0.0;
    const int total = static_cast<int>(seconds);
    const int mins = total / 60;
    const int secs = total % 60;
    return QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0'));
}

}  // namespace ui