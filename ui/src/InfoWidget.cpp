#include "ui/InfoWidget.h"
#include "ui/Theme.h"
#include "ui/ImageLoader.h"
#include "ui/SearchBridge.h"
#include "ui/AppController.h"
#include "ui/QueueBridge.h"
#include "ui/HomepageBridge.h"
#include "ui/TorrentSelectDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QScrollArea>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QDialog>
#include <QSet>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QMouseEvent>
#include <algorithm>

// ─────────────────────────────────────────────────────────────
//  Circular Avatar Widget — for cast members
// ─────────────────────────────────────────────────────────────
class CastAvatar : public QFrame
{
    Q_OBJECT
public:
    explicit CastAvatar(const QVariantMap& member, QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setFixedSize(90, 120);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);

        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(8);
        lay->setAlignment(Qt::AlignHCenter);

        // Avatar with gold ring
        avatar_ = new QLabel(this);
        avatar_->setFixedSize(72, 72);
        avatar_->setStyleSheet(R"(
            QLabel {
                background: #2a2a3a;
                border-radius: 36px;
                border: 2px solid rgba(251, 191, 36, 0.3);
            }
        )");

        // Load profile image
        const QString profileUrl = member.value("profileUrl").toString();
        if (!profileUrl.isEmpty()) {
            ImageLoader::instance()->load(profileUrl, avatar_);
        }

        lay->addWidget(avatar_, 0, Qt::AlignHCenter);

        // Name
        auto* name = new QLabel(member.value("name").toString(), this);
        name->setStyleSheet(R"(
            QLabel {
                color: #e5e7eb;
                font-size: 11px;
                font-weight: 500;
                background: transparent;
            }
        )");
        name->setAlignment(Qt::AlignHCenter);
        name->setWordWrap(true);
        name->setFixedWidth(85);
        lay->addWidget(name);

        // Character name (if available)
        const QString character = member.value("character").toString();
        if (!character.isEmpty()) {
            auto* charName = new QLabel(character, this);
            charName->setStyleSheet(R"(
                QLabel {
                    color: #6b7280;
                    font-size: 10px;
                    font-style: italic;
                    background: transparent;
                }
            )");
            charName->setAlignment(Qt::AlignHCenter);
            charName->setWordWrap(true);
            charName->setFixedWidth(85);
            lay->addWidget(charName);
        }
    }

protected:
    void enterEvent(QEnterEvent* e) override
    {
        // Gold ring glow on hover
        avatar_->setStyleSheet(R"(
            QLabel {
                background: #2a2a3a;
                border-radius: 36px;
                border: 2px solid #fbbf24;
            }
        )");

        // Lift effect
        auto* anim = new QPropertyAnimation(this, "pos", this);
        anim->setDuration(150);
        anim->setStartValue(pos());
        anim->setEndValue(pos() - QPoint(0, 4));
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        QFrame::enterEvent(e);
    }

    void leaveEvent(QEvent* e) override
    {
        avatar_->setStyleSheet(R"(
            QLabel {
                background: #2a2a3a;
                border-radius: 36px;
                border: 2px solid rgba(251, 191, 36, 0.3);
            }
        )");
        QFrame::leaveEvent(e);
    }

private:
    QLabel* avatar_;
};

// ─────────────────────────────────────────────────────────────
//  Similar Movie Card — compact, hoverable
// ─────────────────────────────────────────────────────────────
class SimilarCard : public QFrame
{
    Q_OBJECT
public:
    explicit SimilarCard(const QVariantMap& item, QWidget* parent = nullptr)
        : QFrame(parent), item_(item)
    {
        setFixedSize(140, 210);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);

        applyStyle(false);

        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        // Poster
        poster_ = new QLabel(this);
        poster_->setFixedSize(140, 170);
        poster_->setStyleSheet(R"(
            QLabel {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 #1a1a2e, stop:1 #252538);
                border-radius: 12px;
            }
        )");

        const QString posterUrl = item.value("posterUrl").toString();
        if (!posterUrl.isEmpty()) {
            ImageLoader::instance()->load(posterUrl, poster_);
        }

        lay->addWidget(poster_);

        // Title bar
        auto* titleBar = new QWidget(this);
        titleBar->setStyleSheet("background: transparent;");
        auto* tbLay = new QVBoxLayout(titleBar);
        tbLay->setContentsMargins(8, 6, 8, 6);
        tbLay->setSpacing(2);

        auto* title = new QLabel(item.value("title").toString(), titleBar);
        title->setStyleSheet(R"(
            QLabel {
                color: #e5e7eb;
                font-size: 11px;
                font-weight: 600;
                background: transparent;
            }
        )");
        title->setWordWrap(false);
        tbLay->addWidget(title);

        const double rating = item.value("rating").toDouble();
        if (rating > 0) {
            auto* ratingLbl = new QLabel(QString::fromUtf8("★ %1").arg(rating, 0, 'f', 1), titleBar);
            ratingLbl->setStyleSheet("color: #fbbf24; font-size: 10px; font-weight: 700; background: transparent;");
            tbLay->addWidget(ratingLbl);
        }

        lay->addWidget(titleBar);
    }

    QSize sizeHint() const override { return {140, 210}; }

signals:
    void clicked(const QString& id);

protected:
    void enterEvent(QEnterEvent* e) override
    {
        applyStyle(true);
        auto* shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(20);
        shadow->setColor(QColor(139, 92, 246, 80));
        shadow->setOffset(0, 8);
        setGraphicsEffect(shadow);
        QFrame::enterEvent(e);
    }

    void leaveEvent(QEvent* e) override
    {
        applyStyle(false);
        setGraphicsEffect(nullptr);
        QFrame::leaveEvent(e);
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton) {
            emit clicked(item_.value("id").toString());
        }
        QFrame::mousePressEvent(e);
    }

private:
    void applyStyle(bool hovered)
    {
        if (hovered) {
            setStyleSheet(R"(
                QFrame {
                    background: #1e1e2e;
                    border: 1px solid rgba(139, 92, 246, 0.4);
                    border-radius: 14px;
                }
            )");
        } else {
            setStyleSheet(R"(
                QFrame {
                    background: #181822;
                    border: 1px solid rgba(255, 255, 255, 0.06);
                    border-radius: 14px;
                }
            )");
        }
    }

    QVariantMap item_;
    QLabel* poster_;
};

// ─────────────────────────────────────────────────────────────
//  InfoWidget Implementation
// ─────────────────────────────────────────────────────────────
InfoWidget::InfoWidget(SearchBridge* search,
                       AppController* app,
                       QueueBridge* queue,
                       HomepageBridge* homepage,
                       QWidget* parent)
    : QWidget(parent),
      search_(search), app_(app), queue_(queue), homepage_(homepage)
{
    setObjectName("InfoWidget");
    setStyleSheet(R"(
        QWidget#InfoWidget {
            background: #0a0a0f;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollBar:vertical {
            width: 8px;
            background: transparent;
        }
        QScrollBar::handle:vertical {
            background: #3f3f46;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #52525b;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── GLASS TOP BAR ─────────────────────────────────────
    auto* topBar = new QWidget(this);
    topBar->setFixedHeight(64);
    topBar->setObjectName("infoTopBar");
    topBar->setStyleSheet(R"(
        QWidget#infoTopBar {
            background: rgba(15, 15, 26, 200);
            border-bottom: 1px solid rgba(139, 92, 246, 0.2);
        }
    )");

    auto* tl = new QHBoxLayout(topBar);
    tl->setContentsMargins(24, 0, 24, 0);
    tl->setSpacing(16);

    auto* back = new QPushButton(QString::fromUtf8("←  Back"), topBar);
    back->setFixedSize(110, 38);
    back->setCursor(Qt::PointingHandCursor);
    back->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 15);
            color: white;
            border: 1px solid rgba(255, 255, 255, 30);
            border-radius: 10px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: rgba(139, 92, 246, 100);
            border-color: rgba(139, 92, 246, 150);
        }
    )");
    connect(back, &QPushButton::clicked, this, &InfoWidget::backRequested);
    tl->addWidget(back);

    titleLabel_ = new QLabel(topBar);
    titleLabel_->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 17px;
            font-weight: 700;
            background: transparent;
        }
    )");
    tl->addWidget(titleLabel_, 1);

    badge_ = new QLabel("MOVIE", topBar);
    badge_->setAlignment(Qt::AlignCenter);
    badge_->setFixedHeight(28);
    badge_->setMinimumWidth(80);
    badge_->setStyleSheet(R"(
        QLabel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #7c3aed, stop:1 #8b5cf6);
            color: white;
            font-size: 11px;
            font-weight: 700;
            border-radius: 14px;
            padding: 0 14px;
            letter-spacing: 1px;
        }
    )");
    tl->addWidget(badge_);

    root->addWidget(topBar);

    // ── SCROLL AREA ────────────────────────────────────────
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    root->addWidget(scroll, 1);

    auto* content = new QWidget(scroll);
    content->setObjectName("infoContent");
    content->setStyleSheet("background: transparent;");
    scroll->setWidget(content);

    auto* body = new QVBoxLayout(content);
    body->setContentsMargins(0, 0, 0, 40);
    body->setSpacing(0);

    // ── HERO DETAILS SECTION ──────────────────────────────
    auto* heroSection = new QWidget(content);
    heroSection->setMinimumHeight(420);
    heroSection->setObjectName("heroSection");
    heroSection->setStyleSheet(R"(
        QWidget#heroSection {
            background: #0d0d18;
            border-bottom: 1px solid rgba(139, 92, 246, 40);
        }
    )");

    auto* heroGrid = new QGridLayout(heroSection);
    heroGrid->setContentsMargins(0, 0, 0, 0);

    // Hero content
    auto* heroContent = new QWidget(heroSection);
    heroContent->setStyleSheet("background: transparent;");
    auto* hc = new QVBoxLayout(heroContent);
    hc->setContentsMargins(48, 48, 48, 48);
    hc->setSpacing(20);

    // Main content row: Poster + Details
    auto* contentRow = new QHBoxLayout();
    contentRow->setSpacing(40);

    // Poster with glow effect
    auto* posterContainer = new QWidget(heroContent);
    posterContainer->setStyleSheet("background: transparent;");
    auto* pcLay = new QVBoxLayout(posterContainer);
    pcLay->setContentsMargins(0, 0, 0, 0);

    poster_ = new QLabel(posterContainer);
    poster_->setFixedSize(240, 360);
    poster_->setStyleSheet(R"(
        QLabel {
            background: #1a1a2e;
            border-radius: 16px;
            border: 1px solid rgba(255, 255, 255, 20);
        }
    )");

    // Add shadow to poster
    auto* posterShadow = new QGraphicsDropShadowEffect(poster_);
    posterShadow->setBlurRadius(30);
    posterShadow->setColor(QColor(0, 0, 0, 100));
    posterShadow->setOffset(0, 10);
    poster_->setGraphicsEffect(posterShadow);

    pcLay->addWidget(poster_);
    contentRow->addWidget(posterContainer, 0, Qt::AlignTop);

    // Details panel
    auto* detailsPanel = new QWidget(heroContent);
    detailsPanel->setStyleSheet("background: transparent;");
    auto* dp = new QVBoxLayout(detailsPanel);
    dp->setContentsMargins(0, 0, 0, 0);
    dp->setSpacing(16);

    // Title (hero)
    auto* titleHeader = new QLabel(detailsPanel);
    titleHeader->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 42px;
            font-weight: 800;
            background: transparent;
        }
    )");
    titleHeader->setWordWrap(true);
    dp->addWidget(titleHeader);
    heroTitle_ = titleHeader;

    // Meta badges row
    auto* metaRow = new QWidget(detailsPanel);
    metaRow->setStyleSheet("background: transparent;");
    auto* mr = new QHBoxLayout(metaRow);
    mr->setContentsMargins(0, 0, 0, 0);
    mr->setSpacing(12);

    metaLabel_ = new QLabel(metaRow);
    metaLabel_->setStyleSheet(R"(
        QLabel {
            color: #d8d3ff;
            font-size: 14px;
            font-weight: 500;
            background: transparent;
        }
    )");
    mr->addWidget(metaLabel_);
    mr->addStretch();
    dp->addWidget(metaRow);

    // Source badge
    sourceLabel_ = new QLabel(detailsPanel);
    sourceLabel_->setStyleSheet(R"(
        QLabel {
            color: #8b5cf6;
            font-size: 13px;
            font-weight: 600;
            background: rgba(139, 92, 246, 30);
            border: 1px solid rgba(139, 92, 246, 60);
            border-radius: 6px;
            padding: 4px 10px;
        }
    )");
    sourceLabel_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    dp->addWidget(sourceLabel_);

    // Synopsis
    synopsis_ = new QLabel(detailsPanel);
    synopsis_->setWordWrap(true);
    synopsis_->setStyleSheet(R"(
        QLabel {
            color: rgba(255, 255, 255, 200);
            font-size: 15px;
            background: transparent;
        }
    )");
    synopsis_->setMaximumWidth(600);
    dp->addWidget(synopsis_);

    // Action buttons
    auto* actions = new QHBoxLayout();
    actions->setSpacing(14);

    streamBtn_ = new QPushButton(QString::fromUtf8("▶  Stream Now"), detailsPanel);
    streamBtn_->setFixedSize(170, 50);
    streamBtn_->setCursor(Qt::PointingHandCursor);
    streamBtn_->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #7c3aed, stop:1 #8b5cf6);
            color: white;
            font-weight: 700;
            font-size: 15px;
            border: none;
            border-radius: 12px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #8b5cf6, stop:1 #a78bfa);
        }
        QPushButton:disabled {
            background: #4c3a80;
            color: rgba(255, 255, 255, 100);
        }
    )");
    connect(streamBtn_, &QPushButton::clicked, this, &InfoWidget::onStreamClicked);
    actions->addWidget(streamBtn_);

    downloadBtn_ = new QPushButton(QString::fromUtf8("⬇  Download"), detailsPanel);
    downloadBtn_->setFixedSize(150, 50);
    downloadBtn_->setCursor(Qt::PointingHandCursor);
    downloadBtn_->setStyleSheet(R"(
        QPushButton {
            background: rgba(16, 185, 129, 200);
            color: white;
            font-weight: 600;
            font-size: 15px;
            border: none;
            border-radius: 12px;
        }
        QPushButton:hover {
            background: rgba(16, 185, 129, 255);
        }
        QPushButton:disabled {
            background: rgba(16, 185, 129, 100);
        }
    )");
    connect(downloadBtn_, &QPushButton::clicked, this, &InfoWidget::onDownloadClicked);
    actions->addWidget(downloadBtn_);

    trailerBtn_ = new QPushButton(QString::fromUtf8("🎬  Trailer"), detailsPanel);
    trailerBtn_->setFixedSize(130, 50);
    trailerBtn_->setCursor(Qt::PointingHandCursor);
    trailerBtn_->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 20);
            color: white;
            font-weight: 600;
            font-size: 15px;
            border: 1px solid rgba(255, 255, 255, 40);
            border-radius: 12px;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 35);
        }
    )");
    connect(trailerBtn_, &QPushButton::clicked, this, &InfoWidget::onTrailerClicked);
    actions->addWidget(trailerBtn_);

    actions->addStretch();
    dp->addLayout(actions);

    // Status message
    status_ = new QLabel(detailsPanel);
    status_->setWordWrap(true);
    status_->setStyleSheet(R"(
        QLabel {
            color: #ff8080;
            font-size: 14px;
            background: rgba(255, 100, 100, 20);
            border: 1px solid rgba(255, 100, 100, 40);
            border-radius: 8px;
            padding: 10px 14px;
        }
    )");
    status_->hide();
    dp->addWidget(status_);

    dp->addStretch();
    contentRow->addWidget(detailsPanel, 1);
    hc->addLayout(contentRow);
    heroGrid->addWidget(heroContent, 0, 0);
    heroGrid->setRowStretch(0, 1);
    heroGrid->setColumnStretch(0, 1);
    body->addWidget(heroSection);

    // ── CONTENT SECTIONS (below hero) ─────────────────────
    auto* sections = new QWidget(content);
    sections->setStyleSheet("background: transparent;");
    auto* sl = new QVBoxLayout(sections);
    sl->setContentsMargins(48, 40, 48, 0);
    sl->setSpacing(32);

    // CAST SECTION
    auto* castSection = new QWidget(sections);
    castSection->setStyleSheet("background: transparent;");
    auto* csLay = new QVBoxLayout(castSection);
    csLay->setContentsMargins(0, 0, 0, 0);
    csLay->setSpacing(20);

    castTitle_ = new QLabel("Top Cast", castSection);
    castTitle_->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 22px;
            font-weight: 700;
            background: transparent;
        }
    )");
    csLay->addWidget(castTitle_);

    castHost_ = new QWidget(castSection);
    castHost_->setStyleSheet("background: transparent;");
    castLayout_ = new QGridLayout(castHost_);
    castLayout_->setContentsMargins(0, 0, 0, 0);
    castLayout_->setSpacing(16);
    csLay->addWidget(castHost_);

    sl->addWidget(castSection);

    // SIMILAR SECTION
    auto* similarSection = new QWidget(sections);
    similarSection->setStyleSheet("background: transparent;");
    auto* ssLay = new QVBoxLayout(similarSection);
    ssLay->setContentsMargins(0, 0, 0, 0);
    ssLay->setSpacing(20);

    similarTitle_ = new QLabel("More Like This", similarSection);
    similarTitle_->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 22px;
            font-weight: 700;
            background: transparent;
        }
    )");
    ssLay->addWidget(similarTitle_);

    similarHost_ = new QWidget(similarSection);
    similarHost_->setStyleSheet("background: transparent;");
    similarLayout_ = new QGridLayout(similarHost_);
    similarLayout_->setContentsMargins(0, 0, 0, 0);
    similarLayout_->setSpacing(16);
    ssLay->addWidget(similarHost_);

    sl->addWidget(similarSection);

    // EPISODES SECTION (for series)
    episodesSection_ = new QWidget(sections);
    episodesSection_->setStyleSheet(R"(
        QWidget {
            background: rgba(20, 20, 35, 180);
            border: 1px solid rgba(139, 92, 246, 30);
            border-radius: 16px;
        }
    )");
    auto* epLayout = new QVBoxLayout(episodesSection_);
    epLayout->setContentsMargins(24, 24, 24, 24);
    epLayout->setSpacing(16);

    episodesTitle_ = new QLabel("Episodes", episodesSection_);
    episodesTitle_->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 20px;
            font-weight: 700;
            background: transparent;
        }
    )");
    epLayout->addWidget(episodesTitle_);

    // Season selector row
    auto* seasonRow = new QHBoxLayout();
    seasonRow->setSpacing(12);

    auto* seasonLabel = new QLabel("Season:", episodesSection_);
    seasonLabel->setStyleSheet("color: #e5e7eb; font-size: 14px; background: transparent;");
    seasonRow->addWidget(seasonLabel);

    seasonCombo_ = new QComboBox(episodesSection_);
    seasonCombo_->setFixedHeight(36);
    seasonCombo_->setStyleSheet(R"(
        QComboBox {
            background: rgba(255, 255, 255, 15);
            color: white;
            border: 1px solid rgba(255, 255, 255, 30);
            border-radius: 8px;
            padding: 0 12px;
            font-size: 14px;
        }
        QComboBox::drop-down {
            border: none;
            width: 30px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 6px solid white;
            margin-right: 10px;
        }
        QComboBox QAbstractItemView {
            background: #1a1a2e;
            color: white;
            border: 1px solid rgba(139, 92, 246, 50);
            selection-background-color: #7c3aed;
        }
    )");
    seasonRow->addWidget(seasonCombo_);
    seasonRow->addStretch();
    epLayout->addLayout(seasonRow);

    // Episode list
    episodeList_ = new QListWidget(episodesSection_);
    episodeList_->setStyleSheet(R"(
        QListWidget {
            background: rgba(10, 10, 20, 150);
            border: 1px solid rgba(255, 255, 255, 20);
            border-radius: 12px;
            outline: none;
        }
        QListWidget::item {
            color: #e5e7eb;
            padding: 14px 16px;
            border-bottom: 1px solid rgba(255, 255, 255, 20);
            font-size: 14px;
        }
        QListWidget::item:hover {
            background: rgba(139, 92, 246, 40);
        }
        QListWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #7c3aed, stop:1 #8b5cf6);
            color: white;
        }
    )");
    episodeList_->setFixedHeight(220);
    epLayout->addWidget(episodeList_);

    auto* streamEpBtn = new QPushButton("▶  Stream Selected Episode", episodesSection_);
    streamEpBtn->setFixedSize(220, 46);
    streamEpBtn->setCursor(Qt::PointingHandCursor);
    streamEpBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #7c3aed, stop:1 #8b5cf6);
            color: white;
            font-weight: 600;
            font-size: 14px;
            border: none;
            border-radius: 10px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #8b5cf6, stop:1 #a78bfa);
        }
    )");
    connect(streamEpBtn, &QPushButton::clicked, this, &InfoWidget::onEpisodeStream);
    epLayout->addWidget(streamEpBtn);

    sl->addWidget(episodesSection_);
    body->addWidget(sections);

    // ── SIGNAL CONNECTIONS ────────────────────────────────
    connect(seasonCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        selectedSeason_ = seasonCombo_->currentData().toInt();
        populateEpisodes();
    });

    connect(episodeList_, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0) selectedEpisode_ = row + 1;
    });

    // ── BRIDGE WIRING ─────────────────────────────────────
    if (search_) {
        connect(search_, &SearchBridge::streamUrlReady,
                this, &InfoWidget::onStreamUrlReady);
        connect(search_, &SearchBridge::streamUrlError,
                this, &InfoWidget::onStreamUrlError);
        connect(search_, &SearchBridge::downloadOptionsReady,
                this, &InfoWidget::onDownloadOptionsReady);
    }
    if (queue_) {
        connect(queue_, &QueueBridge::torrentReadyToPlay,
                this, &InfoWidget::onTorrentReadyToPlay);
        connect(queue_, &QueueBridge::streamError,
                this, &InfoWidget::onQueueStreamError);
    }
}

void InfoWidget::clearCast() {
    while (QLayoutItem* item = castLayout_->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
}

void InfoWidget::clearSimilar() {
    while (QLayoutItem* item = similarLayout_->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
}

void InfoWidget::clearEpisodes() {
    seasonCombo_->clear();
    episodeList_->clear();
}

void InfoWidget::populateEpisodes() {
    episodeList_->clear();
    const QVariantList episodes = info_.value("episodes").toList();
    for (const QVariant& e : episodes) {
        const QVariantMap ep = e.toMap();
        if (ep.value("season").toInt() != selectedSeason_) continue;
        const QString label = QString("E%1 — %2")
            .arg(ep.value("episode").toInt())
            .arg(ep.value("title").toString());
        episodeList_->addItem(label);
    }
    if (episodeList_->count() > 0) {
        episodeList_->setCurrentRow(0);
        selectedEpisode_ = 1;
    }
}

void InfoWidget::onEpisodeStream() {
    const QString id = info_.value("id").toString();
    const QString title = info_.value("title").toString();
    const QString url = QString("https://player.vidlove.cc/embed/tv/%1/%2/%3")
        .arg(id).arg(selectedSeason_).arg(selectedEpisode_);
    if (app_) app_->startStream(title, url, "embed");
}

void InfoWidget::updateButtons() {
    if (streamBtn_)
        streamBtn_->setEnabled(!resolving_);
    if (downloadBtn_)
        downloadBtn_->setEnabled(!resolving_);

    if (streamBtn_)
        streamBtn_->setText(resolving_ && pendingAction_ == "stream"
                                ? "⏳  Buffering..."
                                : QString::fromUtf8("▶  Stream Now"));
    if (downloadBtn_)
        downloadBtn_->setText(resolving_ && pendingAction_ == "download"
                                  ? "⏳  Queuing..."
                                  : QString::fromUtf8("⬇  Download"));
}

void InfoWidget::setStatus(const QString& text, bool ok) {
    status_->setText(text);
    status_->setStyleSheet(
        QString(R"(
            QLabel {
                color: %1;
                font-size: 14px;
                background: %2;
                border: 1px solid %3;
                border-radius: 8px;
                padding: 10px 14px;
            }
        )")
            .arg(ok ? "#4ade80" : "#ff8080")
            .arg(ok ? "rgba(74, 222, 128, 20)" : "rgba(255, 100, 100, 20)")
            .arg(ok ? "rgba(74, 222, 128, 40)" : "rgba(255, 100, 100, 40)"));
    status_->show();
}

void InfoWidget::setInfo(const QVariantMap& info) {
    info_ = info;
    const bool loading = !info.contains("synopsis");
    resolving_ = loading;
    pendingAction_.clear();
    status_->hide();
    updateButtons();
    if (loading)
        setStatus("Fetching movie details...", true);

    const bool isSeries = info.value("type").toString() == "tv" ||
                          info.value("type").toString() == "series";

    // ── Top bar title + badge ─────────────────────────────
    titleLabel_->setText(info.value("title").toString());

    badge_->setText(isSeries ? "SERIES" : "MOVIE");
    badge_->setStyleSheet(
        QString(R"(
            QLabel {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 %1, stop:1 %2);
                color: white;
                font-size: 11px;
                font-weight: 700;
                border-radius: 14px;
                padding: 0 14px;
                letter-spacing: 1px;
            }
        )")
            .arg(isSeries ? "#0e7490" : "#7c3aed")
            .arg(isSeries ? "#06b6d4" : "#8b5cf6"));

    // ── Hero title ────────────────────────────────────────
    heroTitle_->setText(info.value("title").toString());

    // ── Poster ────────────────────────────────────────────
    const QString posterUrl = info.value("posterUrl").toString();
    if (posterUrl.isEmpty()) {
        poster_->clear();
        poster_->setText(QString::fromUtf8("🎬"));
        poster_->setAlignment(Qt::AlignCenter);
        poster_->setStyleSheet(R"(
            QLabel {
                background: #1a1a2e;
                border-radius: 16px;
                border: 1px solid rgba(255, 255, 255, 20);
                font-size: 64px;
            }
        )");
    } else {
        poster_->setText({});
        poster_->setStyleSheet(R"(
            QLabel {
                background: #1a1a2e;
                border-radius: 16px;
                border: 1px solid rgba(255, 255, 255, 20);
            }
        )");
        ImageLoader::instance()->load(posterUrl, poster_);
    }

    // ── Meta row ──────────────────────────────────────────
    QStringList meta;
    const int year = info.value("year").toInt();
    if (year > 0) meta << QString::number(year);

    const int mins = info.value("lengthMins").toInt();
    if (mins > 0)
        meta << QString("%1h %2m").arg(mins / 60).arg(mins % 60);

    const QString quality = info.value("quality").toString();
    if (!quality.isEmpty()) meta << quality;

    const double rating = info.value("rating").toDouble();
    if (rating > 0)
        meta << QString::fromUtf8("★ %1").arg(rating, 0, 'f', 1);

    metaLabel_->setText(meta.join("   ·   "));
    sourceLabel_->setText(info.value("sourceName").toString());
    sourceLabel_->setVisible(!info.value("sourceName").toString().isEmpty());

    const QString syn = info.value("synopsis").toString();
    synopsis_->setText(syn);
    synopsis_->setVisible(!syn.isEmpty());

    streamBtn_->setVisible(info.value("hasStream").toBool());
    downloadBtn_->setVisible(info.value("hasDownload").toBool());

    const QString trailer = info.value("trailerUrl").toString();
    trailerBtn_->setVisible(!trailer.isEmpty());

    // ── CAST ──────────────────────────────────────────────
    clearCast();
    clearSimilar();
    clearEpisodes();

    const QVariantList cast = info.value("cast").toList();
    castTitle_->setVisible(!cast.isEmpty());
    castHost_->setVisible(!cast.isEmpty());

    int col = 0, row = 0;
    const int maxCols = 8;
    for (const QVariant& c : cast) {
        const QVariantMap member = c.toMap();
        auto* avatar = new CastAvatar(member, castHost_);
        castLayout_->addWidget(avatar, row, col);
        if (++col >= maxCols) {
            col = 0;
            ++row;
        }
    }

    // ── SIMILAR ───────────────────────────────────────────
    const QVariantList similar = info.value("similar").toList();
    similarTitle_->setVisible(!similar.isEmpty());
    similarHost_->setVisible(!similar.isEmpty());

    int scol = 0, srow = 0;
    const int smaxCols = 6;
    for (const QVariant& s : similar) {
        const QVariantMap item = s.toMap();
        auto* card = new SimilarCard(item, similarHost_);
        // TODO: Connect card click to load that movie's info
        similarLayout_->addWidget(card, srow, scol);
        if (++scol >= smaxCols) { scol = 0; ++srow; }
    }

    // ── EPISODES ──────────────────────────────────────────
    episodesSection_->setVisible(isSeries);
    if (isSeries) {
        QSet<int> seasons;
        for (const QVariant& e : info.value("episodes").toList())
            seasons.insert(e.toMap().value("season").toInt());

        QList<int> sortedSeasons = seasons.values();
        std::sort(sortedSeasons.begin(), sortedSeasons.end());
        for (int season : sortedSeasons)
            seasonCombo_->addItem(QString("Season %1").arg(season), season);

        selectedSeason_ = sortedSeasons.isEmpty() ? 1 : sortedSeasons.first();
        populateEpisodes();
    }
}

// ─────────────────────────────────────────────────────────────
//  Action handlers (unchanged from original)
// ─────────────────────────────────────────────────────────────
void InfoWidget::onStreamClicked() {
    pendingAction_ = "stream";
    resolving_ = true;
    status_->hide();
    updateButtons();
    if (search_) {
        search_->getStreamUrl(info_.value("id").toString(),
                              info_.value("sourceName").toString(),
                              info_.value("title").toString());
    }
}

void InfoWidget::onDownloadClicked() {
    pendingAction_ = "downloadOptions";
    resolving_ = true;
    status_->hide();
    updateButtons();
    if (search_) {
        const QString type = info_.value("type").toString();
        const bool isTV = type == "tv" || type == "series";
        search_->getDownloadOptions(info_.value("title").toString(),
                                    info_.value("year").toInt(), isTV);
    }
}

void InfoWidget::onDownloadOptionsReady(const QVariantList& options) {
    if (pendingAction_ != "downloadOptions") return;

    if (options.isEmpty()) {
        resolving_ = false;
        pendingAction_.clear();
        updateButtons();
        setStatus("No downloadable torrents found.", false);
        return;
    }

    TorrentSelectDialog dialog(this);
    dialog.setOptions(options);
    if (dialog.exec() == QDialog::Accepted) {
        const QString magnetUrl = dialog.selectedMagnetUrl();
        if (queue_ && !magnetUrl.isEmpty()) {
            queue_->startMetadataFetch(magnetUrl);
            setStatus("Fetching file list... check Downloads tab.", true);
        }
    }

    resolving_ = false;
    pendingAction_.clear();
    updateButtons();
}

void InfoWidget::onTrailerClicked() {
    const QString url = info_.value("trailerUrl").toString();
    if (!url.isEmpty()) QDesktopServices::openUrl(QUrl(url));
}

void InfoWidget::onStreamUrlReady(const QString& title, const QString& url) {
    if (!resolving_) return;

    if (pendingAction_ == "download") {
        if (url.startsWith("magnet:", Qt::CaseInsensitive)) {
            pendingMagnetTitle_ = title;
            if (queue_) queue_->startMetadataFetch(url);
            setStatus("Fetching file list... check Downloads tab.", true);
        } else {
            const bool ok = queue_ ? queue_->enqueue(title, url) : false;
            setStatus(ok ? "Added to Downloads." : "Couldn't start download.", ok);
        }
        resolving_ = false;
        pendingAction_.clear();
        updateButtons();
        return;
    }

    const QString sType = info_.value("streamType").toString();
    if (sType == "torrent") {
        setStatus("Buffering torrent... will play when ready.", true);
        if (queue_) queue_->streamTorrent(title, url);
        // resolving_ stays true until torrentReadyToPlay fires
    } else {
        resolving_ = false;
        pendingAction_.clear();
        updateButtons();
        if (app_) app_->startStream(title, url, info_.value("streamType").toString());
    }
}

void InfoWidget::onStreamUrlError(const QString& message) {
    if (!resolving_) return;
    resolving_ = false;
    pendingAction_.clear();
    updateButtons();
    setStatus("Error: " + message, false);
}

void InfoWidget::onTorrentReadyToPlay(const QString& title, const QString& filePath) {
    if (pendingAction_ != "stream") return;
    resolving_ = false;
    pendingAction_.clear();
    updateButtons();
    status_->hide();

    // Subtitles — if the source advertises them, hand each one to the player.
    if (info_.value("hasSubtitles").toBool() && homepage_) {
        const QVariantList subs = homepage_->getSubtitleUrls(
            info_.value("id").toString(), info_.value("sourceName").toString());
        for (const QVariant& s : subs) {
            // playerModule.loadSubtitle(s.toString());
            Q_UNUSED(s);
        }
    }

    if (app_) app_->startStream(title, filePath, info_.value("streamType").toString());
}

void InfoWidget::onQueueStreamError(const QString& message) {
    resolving_ = false;
    pendingAction_.clear();
    updateButtons();
    setStatus("Stream error: " + message, false);
}

#include "InfoWidget.moc"