#include "ui/PlayerPageWidget.h"
#include "ui/VideoSurfaceWidget.h"
#include "queue_manager/queue_manager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QStyle>
#include <algorithm>
#include <cmath>

// ── IPC request IDs ──────────────────────────────────────────
static constexpr int kReqPosition = 1;
static constexpr int kReqDuration = 2;
static constexpr int kReqPause    = 3;
static constexpr int kReqVolume   = 4;

// ── Timings ──────────────────────────────────────────────────
constexpr int    kIdleHideMs          = 3500;
constexpr int    kDoubleTapMs         = 400;
constexpr double kSeekSmallSeconds    = 10.0;
constexpr double kSeekDoubleTapSeconds= 20.0;

// ── Click-to-seek slider ─────────────────────────────────────
class ClickToSeekSlider : public QSlider {
public:
    explicit ClickToSeekSlider(Qt::Orientation o, QWidget* p = nullptr)
        : QSlider(o, p) {}
protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton)
            setValue(QStyle::sliderValueFromPosition(minimum(), maximum(),
                                                     e->pos().x(), width()));
        QSlider::mousePressEvent(e);
    }
};

// ════════════════════════════════════════════════════════════
// Constructor
// ════════════════════════════════════════════════════════════
PlayerPageWidget::PlayerPageWidget(
    std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
    QWidget* parent)
    : QWidget(parent), queueManager_(std::move(queueManager))
{
    setStyleSheet("background-color: #000000;");
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    surface_ = new VideoSurfaceWidget(this);
    surface_->setGeometry(rect());
    surface_->lower();

    // ── Info overlay ─────────────────────────────────────────
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

    // ── Controls overlay ─────────────────────────────────────
    controlsOverlay_ = new QWidget(this);
    controlsOverlay_->setStyleSheet(
        "background-color: rgba(0,0,0,160); border-radius: 6px;");

    playPauseButton_ = new QPushButton(controlsOverlay_);
    playPauseButton_->setFixedWidth(40);
    updatePlayPauseIcon(false);
    connect(playPauseButton_, &QPushButton::clicked,
            this, &PlayerPageWidget::onPlayPauseClicked);

    elapsedLabel_  = new QLabel("0:00", controlsOverlay_);
    durationLabel_ = new QLabel("0:00", controlsOverlay_);
    elapsedLabel_ ->setStyleSheet("color: #cccccc; font-size: 12px;");
    durationLabel_->setStyleSheet("color: #cccccc; font-size: 12px;");

    seekSlider_ = new ClickToSeekSlider(Qt::Horizontal, controlsOverlay_);
    seekSlider_->setRange(0, 1000);
    seekSlider_->setValue(0);
    connect(seekSlider_, &QSlider::sliderPressed,
            this, &PlayerPageWidget::onSeekSliderPressed);
    connect(seekSlider_, &QSlider::sliderReleased,
            this, &PlayerPageWidget::onSeekSliderReleased);

    auto* volumeIcon = new QLabel("🔊", controlsOverlay_);
    volumeSlider_ = new QSlider(Qt::Horizontal, controlsOverlay_);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(100);
    volumeSlider_->setFixedWidth(90);
    connect(volumeSlider_, &QSlider::valueChanged,
            this, &PlayerPageWidget::onVolumeChanged);

    downloadButton_ = new QPushButton("⬇ Download", controlsOverlay_);
    downloadButton_->setToolTip("Save this to Downloads while it plays");
    downloadButton_->setVisible(false);
    connect(downloadButton_, &QPushButton::clicked,
            this, &PlayerPageWidget::onDownloadClicked);

    fullscreenButton_ = new QPushButton("⛶", controlsOverlay_);
    fullscreenButton_->setFixedWidth(36);
    connect(fullscreenButton_, &QPushButton::clicked,
            this, &PlayerPageWidget::onFullscreenClicked);

    helpButton_ = new QPushButton("?", controlsOverlay_);
    helpButton_->setFixedWidth(30);
    helpButton_->setToolTip("Show keyboard shortcuts");
    connect(helpButton_, &QPushButton::clicked,
            this, [this]() { toggleHelpOverlay(); });

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
    toolsRow->addStretch(1);
    toolsRow->addWidget(downloadButton_);
    toolsRow->addWidget(fullscreenButton_);
    toolsRow->addWidget(helpButton_);

    auto* controlsLayout = new QVBoxLayout(controlsOverlay_);
    controlsLayout->setContentsMargins(16, 8, 16, 10);
    controlsLayout->setSpacing(6);
    controlsLayout->addLayout(seekRow);
    controlsLayout->addLayout(toolsRow);

    // ── Help overlay ─────────────────────────────────────────
    helpOverlay_ = new QWidget(this);
    helpOverlay_->setStyleSheet(
        "background-color: rgba(0,0,0,200); border-radius: 8px;");
    helpOverlay_->hide();

    auto* helpLabel = new QLabel(helpOverlay_);
    helpLabel->setStyleSheet("color: #eeeeee; font-size: 13px;");
    helpLabel->setTextFormat(Qt::RichText);
    helpLabel->setText(
        "<b>Controls</b><br>"
        "Space &nbsp;&nbsp; Play / Pause<br>"
        "&larr; / &rarr; &nbsp;&nbsp; Seek ±10s (double &rarr; = +20s)<br>"
        "&uarr; / &darr; &nbsp;&nbsp; Volume +5 / -5<br>"
        "F &nbsp;&nbsp; Toggle fullscreen<br>"
        "Esc &nbsp;&nbsp; Exit fullscreen<br>"
        "H / ? &nbsp;&nbsp; Toggle this help<br>");
    auto* helpLayout = new QVBoxLayout(helpOverlay_);
    helpLayout->setContentsMargins(20, 16, 20, 16);
    helpLayout->addWidget(helpLabel);

    // ── Top bar ───────────────────────────────────────────────
    topBarWidget_ = new QWidget(this);
    topBarWidget_->setStyleSheet(
        "background: rgba(0,0,0,150); border-radius: 4px;");
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

    // ── Styling ───────────────────────────────────────────────
    setStyleSheet(styleSheet() +
        "QPushButton { background-color: #171225; color: white;"
        "  border: 1px solid #7c3aed; border-radius: 6px; padding: 4px 10px; }"
        "QPushButton:hover { background-color: #2a2140; }"
        "QSlider::groove:horizontal { background: #2a2a3a; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #6fae6f; width: 12px; height: 12px;"
        "  margin: -4px 0; border-radius: 6px; }"
        "QSlider::sub-page:horizontal { background: #6fae6f; border-radius: 2px; }");

    topBarWidget_->raise();
    infoOverlay_->raise();
    controlsOverlay_->raise();
    helpOverlay_->raise();

    // ── Timers ────────────────────────────────────────────────
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

PlayerPageWidget::~PlayerPageWidget() {
    stopStream();
}

// ════════════════════════════════════════════════════════════
// Playback
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::startStream(const QString& title, const QString& url) {
    stopStream();

    titleLabel_->setText(title);
    infoTitleLabel_->setText(title);
    infoPathLabel_->setText(url);
    currentStreamTitle_ = title;
    currentStreamUrl_   = url;

    downloadButton_->setVisible(isRemoteUrl(url));
    downloadButton_->setEnabled(true);
    downloadButton_->setText("⬇ Download");

    waitingOnQueue_  = false;
    pendingQueueId_.clear();

    if (url.startsWith("magnet:", Qt::CaseInsensitive)) {
        setStatus("Queuing torrent...");
        pendingQueueId_ = QString::fromStdString(
            queueManager_->enqueueTorrent(title.toStdString(), url.toStdString()));
        if (pendingQueueId_.isEmpty()) {
            setStatus("Failed to queue torrent.");
            return;
        }
        waitingOnQueue_ = true;
        return;
    }

    beginPlayback(url);
    showOverlays();
}

void PlayerPageWidget::beginPlayback(const QString& path) {
    setStatus("Loading...");
    surface_->player().load(path.toStdString());
    surface_->player().play();
    showOverlays();
}

void PlayerPageWidget::stopStream() {
    waitingOnQueue_ = false;
    pendingQueueId_.clear();

    surface_->player().stop();

    isPlaying_ = false;
    position_  = 0.0;
    duration_  = 0.0;
    updatePlayPauseIcon(false);
    seekSlider_->setValue(0);
    elapsedLabel_->setText("0:00");
    durationLabel_->setText("0:00");
    setStatus(QString());
}

// ════════════════════════════════════════════════════════════
// Tick — poll queue + update UI from IPC state
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::onTick() {
    if (queueManager_) queueManager_->update();

    // torrent waiting
    if (waitingOnQueue_ && !pendingQueueId_.isEmpty()) {
        const auto item = queueManager_->getItem(pendingQueueId_.toStdString());
        if (item.state == queue_manager::QueueItemState::Error) {
            waitingOnQueue_ = false;
            setStatus("Torrent failed to start.");
        } else if (item.readyToPlay && !item.filePath.empty()) {
            waitingOnQueue_ = false;
            beginPlayback(QString::fromStdString(item.filePath));
        } else {
            const int pct = static_cast<int>(item.progress * 100.0);
            setStatus(QString("Buffering... %1%").arg(pct));
        }
        return;
    }

    surface_->player().update();
    const auto playback = surface_->player().getStatus();
    position_ = playback.positionSeconds;
    duration_ = playback.durationSeconds;
    volume_ = playback.volume;
    isPlaying_ = playback.state == player::PlaybackState::Playing;
    if (playback.state == player::PlaybackState::Error)
        setStatus("Playback failed. Check that libmpv is installed.");
    else if (playback.state == player::PlaybackState::Playing)
        setStatus(QString());

    // update seek slider
    if (!userScrubbing_ && duration_ > 0.0) {
        seekSlider_->setValue(static_cast<int>((position_ / duration_) * 1000.0));
    }
    elapsedLabel_->setText(formatTime(position_));
    durationLabel_->setText(formatTime(duration_));
    updatePlayPauseIcon(isPlaying_);
}

// ════════════════════════════════════════════════════════════
// Controls
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::togglePlayPause() {
    if (isPlaying_) surface_->player().pause();
    else            surface_->player().play();
    showOverlays();
    armIdleHideTimer();
}

void PlayerPageWidget::onPlayPauseClicked() { togglePlayPause(); }

void PlayerPageWidget::onSeekSliderPressed()  { userScrubbing_ = true; }

void PlayerPageWidget::onSeekSliderReleased() {
    if (duration_ > 0.0) {
        const double target = (seekSlider_->value() / 1000.0) * duration_;
        surface_->player().seek(target);
    }
    userScrubbing_ = false;
    showOverlays();
    armIdleHideTimer();
}

void PlayerPageWidget::onVolumeChanged(int value) {
    volume_ = value;
    surface_->player().setVolume(value);
    showOverlays();
    armIdleHideTimer();
}

void PlayerPageWidget::skipSeconds(double seconds) {
    surface_->player().seek(position_ + seconds);
}

void PlayerPageWidget::adjustVolume(double delta) {
    const int newVal = std::clamp(volumeSlider_->value() + static_cast<int>(delta), 0, 100);
    volumeSlider_->setValue(newVal); // triggers onVolumeChanged
    bufferingLabel_->setText(QString("🔊 %1%").arg(newVal));
    bufferingLabel_->show();
    QTimer::singleShot(1500, this, [this]() {
        if (bufferingLabel_->text().startsWith("🔊")) bufferingLabel_->hide();
    });
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

// ════════════════════════════════════════════════════════════
// Fullscreen
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::onFullscreenClicked() { toggleFullscreen(); }

void PlayerPageWidget::toggleFullscreen() {
    auto* win = window();
    if (!win) return;
    if (!wasFullscreen_) { win->showFullScreen(); fullscreenButton_->setText("🗗"); }
    else                 { win->showNormal();      fullscreenButton_->setText("⛶"); }
    wasFullscreen_ = !wasFullscreen_;
}

// ════════════════════════════════════════════════════════════
// Keyboard
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::keyPressEvent(QKeyEvent* event) {
    showOverlays();
    armIdleHideTimer();

    switch (event->key()) {
        case Qt::Key_Space: togglePlayPause(); break;
        case Qt::Key_Right:
            if (lastRightArrowPress_.isValid() &&
                lastRightArrowPress_.elapsed() < kDoubleTapMs)
                skipSeconds(kSeekDoubleTapSeconds);
            else
                skipSeconds(kSeekSmallSeconds);
            lastRightArrowPress_.restart();
            break;
        case Qt::Key_Left:  skipSeconds(-kSeekSmallSeconds); break;
        case Qt::Key_Up:    adjustVolume(5.0);  break;
        case Qt::Key_Down:  adjustVolume(-5.0); break;
        case Qt::Key_F:     toggleFullscreen(); break;
        case Qt::Key_Escape:
            if (wasFullscreen_) toggleFullscreen(); break;
        case Qt::Key_H:
        case Qt::Key_Question: toggleHelpOverlay(); break;
        default: QWidget::keyPressEvent(event); return;
    }
    event->accept();
}

// ════════════════════════════════════════════════════════════
// Mouse
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::mouseMoveEvent(QMouseEvent* event) {
    showOverlays();
    armIdleHideTimer();
    QWidget::mouseMoveEvent(event);
}

void PlayerPageWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // only toggle if click is NOT on an overlay widget
        const QPoint pos = event->pos();
        if (!controlsOverlay_->geometry().contains(pos) &&
            !topBarWidget_->geometry().contains(pos)) {
            togglePlayPause();
        }
    }
    showOverlays();
    armIdleHideTimer();
    QWidget::mousePressEvent(event);
}

void PlayerPageWidget::wheelEvent(QWheelEvent* event) {
    const int steps = event->angleDelta().y() / 120;
    if (steps != 0) adjustVolume(steps * 5.0);
    showOverlays();
    armIdleHideTimer();
}

// ════════════════════════════════════════════════════════════
// Overlay management
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::showOverlays() {
    overlaysVisible_ = true;
    topBarWidget_->show();
    infoOverlay_->show();
    controlsOverlay_->show();
    unsetCursor();
}

void PlayerPageWidget::armIdleHideTimer() {
    if (!helpVisible_) idleHideTimer_->start(kIdleHideMs);
}

void PlayerPageWidget::toggleHelpOverlay() {
    helpVisible_ = !helpVisible_;
    helpOverlay_->setVisible(helpVisible_);
    if (helpVisible_) { helpOverlay_->raise(); idleHideTimer_->stop(); showOverlays(); }
    else              { armIdleHideTimer(); }
}

// ════════════════════════════════════════════════════════════
// Resize
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    surface_->setGeometry(rect());
    topBarWidget_->setGeometry(10, 10, width() - 20, 44);
    infoOverlay_->setGeometry(10, 60, width() - 20, 100);
    const int ch = 100;
    controlsOverlay_->setGeometry(10, height() - ch - 10, width() - 20, ch);
    helpOverlay_->setGeometry((width()-340)/2, (height()-220)/2, 340, 220);
}

// ════════════════════════════════════════════════════════════
// Helpers
// ════════════════════════════════════════════════════════════
void PlayerPageWidget::setStatus(const QString& text) {
    statusLabel_->setText(text);
    statusLabel_->setVisible(!text.isEmpty());
    if (text.isEmpty()) bufferingLabel_->hide();
    else { bufferingLabel_->setText(text); bufferingLabel_->show(); }
}

void PlayerPageWidget::updatePlayPauseIcon(bool playing) {
    playPauseButton_->setText(playing ? "⏸" : "▶");
}

QString PlayerPageWidget::formatTime(double seconds) {
    if (seconds < 0.0 || std::isnan(seconds)) seconds = 0.0;
    const int total = static_cast<int>(seconds);
    return QString("%1:%2").arg(total / 60).arg(total % 60, 2, 10, QChar('0'));
}

bool PlayerPageWidget::isRemoteUrl(const QString& url) {
    return url.startsWith("http://",  Qt::CaseInsensitive) ||
           url.startsWith("https://", Qt::CaseInsensitive);
}