#include "ui/PlayerPageWidget.h"
#include "ui/VideoSurfaceWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QStyle>
#include <QGuiApplication>
#include <QScreen>
#include <algorithm>
#include <cmath>

namespace ui {

namespace {

// Plain QSlider jumping a page-step on click. This subclass jumps
// straight to wherever you clicked, which is what "seeking" should
// feel like on a scrub bar.
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

}  // namespace

PlayerPageWidget::PlayerPageWidget(std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                                   QWidget* parent)
    : QWidget(parent), queueManager_(std::move(queueManager)) {
    // A normal QWidget -- mpv renders into it as a texture via the render
    // API (see VideoSurfaceWidget/PlayerModule::initGL). No native window
    // handle changes hands, so click-to-pause and scroll-to-adjust-volume
    // are hooked directly on this widget, not via an eventFilter on a
    // foreign QWindow.
    videoWidget_ = new VideoSurfaceWidget(this);
    videoWidget_->setFocusPolicy(Qt::StrongFocus);
    videoWidget_->installEventFilter(this);

    titleLabel_ = new QLabel(this);
    titleLabel_->setStyleSheet("color: white; font-size: 16px;");

    statusLabel_ = new QLabel(this);
    statusLabel_->setStyleSheet("color: #aaaaaa; font-size: 13px;");
    statusLabel_->hide();

    backButton_ = new QPushButton("< Back", this);
    connect(backButton_, &QPushButton::clicked, this, [this]() {
        stopStream();
        emit backRequested();
    });

    topBarWidget_ = new QWidget(this);
    auto* topBar = new QHBoxLayout(topBarWidget_);
    topBar->setContentsMargins(0, 0, 0, 0);
    topBar->addWidget(backButton_);
    topBar->addWidget(titleLabel_, /*stretch=*/1);
    topBar->addWidget(statusLabel_);

    // --- Playback control bar ---
    playPauseButton_ = new QPushButton(this);
    playPauseButton_->setFixedWidth(44);
    updatePlayPauseIcon(/*isPlaying=*/false);
    connect(playPauseButton_, &QPushButton::clicked, this, &PlayerPageWidget::onPlayPauseClicked);

    elapsedLabel_ = new QLabel("0:00", this);
    elapsedLabel_->setStyleSheet("color: #cccccc; font-size: 12px;");

    durationLabel_ = new QLabel("0:00", this);
    durationLabel_->setStyleSheet("color: #cccccc; font-size: 12px;");

    seekSlider_ = new ClickToSeekSlider(Qt::Horizontal, this);
    seekSlider_->setRange(0, 1000);
    seekSlider_->setValue(0);
    connect(seekSlider_, &QSlider::sliderPressed, this, &PlayerPageWidget::onSeekSliderPressed);
    connect(seekSlider_, &QSlider::sliderReleased, this, &PlayerPageWidget::onSeekSliderReleased);

    auto* volumeIcon = new QLabel("🔊", this);
    volumeSlider_ = new QSlider(Qt::Horizontal, this);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(100);
    volumeSlider_->setFixedWidth(90);
    connect(volumeSlider_, &QSlider::valueChanged, this, &PlayerPageWidget::onVolumeChanged);

    auto* brightnessIcon = new QLabel("🔆", this);
    brightnessSlider_ = new QSlider(Qt::Horizontal, this);
    // mpv's "brightness" is a raw additive pixel offset, not a gamma/exposure
    // curve -- the full -100..100 range it accepts blows the picture out to
    // a flat white/black well before the slider hits either end. Clamp the
    // UI range so the whole slider stays inside the part of the curve that
    // still looks like a picture.
    brightnessSlider_->setRange(-60, 60);
    brightnessSlider_->setValue(0);
    brightnessSlider_->setFixedWidth(90);
    connect(brightnessSlider_, &QSlider::valueChanged, this, &PlayerPageWidget::onBrightnessChanged);

    downloadButton_ = new QPushButton("⬇ Download", this);
    downloadButton_->setToolTip("Save this to Downloads while it plays");
    downloadButton_->setVisible(false);
    connect(downloadButton_, &QPushButton::clicked, this, &PlayerPageWidget::onDownloadClicked);

    pipButton_ = new QPushButton("🗔", this);
    pipButton_->setFixedWidth(36);
    pipButton_->setToolTip("Picture-in-picture: keep watching while you browse");
    connect(pipButton_, &QPushButton::clicked, this, &PlayerPageWidget::onPipClicked);

    fullscreenButton_ = new QPushButton("⛶", this);
    fullscreenButton_->setFixedWidth(36);
    connect(fullscreenButton_, &QPushButton::clicked, this, &PlayerPageWidget::onFullscreenClicked);

    auto* seekRow = new QHBoxLayout();
    seekRow->setSpacing(10);
    seekRow->addWidget(playPauseButton_);
    seekRow->addWidget(elapsedLabel_);
    seekRow->addWidget(seekSlider_, /*stretch=*/1);
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
    toolsRow->addWidget(pipButton_);
    toolsRow->addWidget(fullscreenButton_);

    controlBarWidget_ = new QWidget(this);
    auto* controlBar = new QVBoxLayout(controlBarWidget_);
    controlBar->setContentsMargins(0, 0, 0, 0);
    controlBar->setSpacing(8);
    controlBar->addLayout(seekRow);
    controlBar->addLayout(toolsRow);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(topBarWidget_);
    layout->addWidget(videoWidget_, /*stretch=*/1);
    layout->addWidget(controlBarWidget_);

    setStyleSheet(
        "background-color: #000000;"
        "QPushButton { background-color: #171225; color: white; border: 1px solid #7c3aed;"
        "              border-radius: 6px; padding: 4px 10px; }"
        "QPushButton:hover { background-color: #2a2140; }"
        "QSlider::groove:horizontal { background: #2a2a3a; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #7c3aed; width: 12px; height: 12px;"
        "                             margin: -4px 0; border-radius: 6px; }"
        "QSlider::sub-page:horizontal { background: #7c3aed; border-radius: 2px; }"
    );

    // Pumps mpv's event queue and, while a magnet is queued, checks
    // queue_manager for readyToPlay. 500ms matches queue_manager's own
    // documented polling cadence for torrent/download progress.
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &PlayerPageWidget::onTick);
    timer_->start(500);
}

PlayerPageWidget::~PlayerPageWidget() = default;

player::PlayerModule& PlayerPageWidget::player() {
    return videoWidget_->player();
}

void PlayerPageWidget::setStatus(const QString& text) {
    statusLabel_->setText(text);
    statusLabel_->setVisible(!text.isEmpty());
}

void PlayerPageWidget::startStream(const QString& title, const QString& url) {
    titleLabel_->setText(title);
    waitingOnQueue_ = false;
    pendingQueueId_.clear();

    currentStreamTitle_ = title;
    currentStreamUrl_ = url;
    downloadButton_->setEnabled(true);
    downloadButton_->setText("⬇ Download");
    // A magnet link is already being pulled to disk via queue_manager just
    // to play it, and a local file (e.g. from Downloads' Play button) is
    // already downloaded -- only offer "Download" for a plain remote link
    // that would otherwise vanish once playback stops.
    downloadButton_->setVisible(isRemoteUrl(url));

    const std::string urlStd = url.toStdString();
    if (urlStd.rfind("magnet:", 0) == 0) {
        setStatus("Queuing torrent...");
        pendingQueueId_ = queueManager_->enqueueTorrent(title.toStdString(), urlStd);
        if (pendingQueueId_.empty()) {
            setStatus("Failed to queue torrent.");
            return;
        }
        waitingOnQueue_ = true;
    } else {
        // Plain HTTP(S) direct link -- no queue, straight into mpv.
        beginPlayback(urlStd);
    }
}

void PlayerPageWidget::beginPlayback(const std::string& path) {
    setStatus("Loading...");
    player().load(path);
    player().play();
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

    if (waitingOnQueue_ && !pendingQueueId_.empty()) {
        const auto item = queueManager_->getItem(pendingQueueId_);
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

    // Don't fight the user while they're dragging the seek handle.
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
}

void PlayerPageWidget::onVolumeChanged(int value) {
    player().setVolume(static_cast<double>(value));
}

void PlayerPageWidget::onBrightnessChanged(int value) {
    player().setBrightness(static_cast<double>(value));
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

void PlayerPageWidget::onFullscreenClicked() {
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

void PlayerPageWidget::onPipClicked() {
    if (inPip_) {
        exitPip();
    } else {
        enterPip();
    }
}

void PlayerPageWidget::enterPip() {
    // A genuine separate OS-level top-level window (not an overlay inside
    // MainWindow) so the window manager handles always-on-top the normal
    // way. The mini player stays video-only for now -- now that videoWidget_
    // is a plain QWidget (not a foreign native surface), a real control bar
    // could be reparented in here too without the click-stealing issues the
    // old wid-embedded version had. Tap the video to come back to the full
    // player in the meantime.
    if (!pipWindow_) {
        pipWindow_ = new QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        pipWindow_->setStyleSheet("background-color: #000000; border: 1px solid #7c3aed;");
        auto* pipLayout = new QVBoxLayout(pipWindow_);
        pipLayout->setContentsMargins(1, 1, 1, 1);
        pipLayout->setSpacing(0);
    }

    auto* pipLayout = static_cast<QVBoxLayout*>(pipWindow_->layout());
    pipLayout->addWidget(videoWidget_, /*stretch=*/1);

    pipWindow_->resize(380, 214);
    if (auto* screen = QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        pipWindow_->move(avail.right() - 380 - 24, avail.bottom() - 214 - 24);
    }
    pipButton_->setText("🗕");
    pipWindow_->show();

    inPip_ = true;
    emit pipActiveChanged(true);
}

void PlayerPageWidget::exitPip() {
    if (!pipWindow_) return;

    auto* mainLayout = static_cast<QVBoxLayout*>(layout());
    mainLayout->insertWidget(1, videoWidget_, /*stretch=*/1);

    pipButton_->setText("🗔");
    pipWindow_->hide();

    inPip_ = false;
    emit pipActiveChanged(false);
}

bool PlayerPageWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == videoWidget_) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (inPip_) {
                    // The mini player intentionally has no button row (see
                    // enterPip) -- tap the video itself to come back to the
                    // full player where Back/volume/everything works.
                    exitPip();
                } else {
                    togglePlayPause();
                }
                return true;
            }
        } else if (event->type() == QEvent::Wheel) {
            auto* wheelEvent = static_cast<QWheelEvent*>(event);
            const int steps = wheelEvent->angleDelta().y() / 120; // one notch = 120
            if (steps != 0) {
                const int newValue = std::clamp(volumeSlider_->value() + steps * 5, 0, 100);
                volumeSlider_->setValue(newValue); // triggers onVolumeChanged via signal
            }
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

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
