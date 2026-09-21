#include "ui/HomeWidget.h"
#include "ui/Theme.h"
#include "ui/ImageLoader.h"
#include "ui/HomepageBridge.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>
#include <QTimer>

// ─────────────────────────────────────────────────────────────
//  ANIMATED HOVER FRAME — gives that "lift" feeling
// ─────────────────────────────────────────────────────────────
class HoverFrame : public QFrame
{
    Q_OBJECT
public:
    explicit HoverFrame(QWidget* parent = nullptr) : QFrame(parent)
    {
        setAttribute(Qt::WA_Hover);
        setCursor(Qt::PointingHandCursor);
        
        // Shadow effect for depth
        shadow_ = new QGraphicsDropShadowEffect(this);
        shadow_->setBlurRadius(0);
        shadow_->setColor(QColor(0, 0, 0, 80));
        shadow_->setOffset(0, 0);
        setGraphicsEffect(shadow_);
    }

protected:
    void enterEvent(QEnterEvent* e) override
    {
        animateShadow(20, QColor(139, 92, 246, 60), QPoint(0, 8));
        QFrame::enterEvent(e);
    }
    
    void leaveEvent(QEvent* e) override
    {
        animateShadow(0, QColor(0, 0, 0, 80), QPoint(0, 0));
        QFrame::leaveEvent(e);
    }

private:
    void animateShadow(int blur, QColor color, QPoint offset)
    {
        auto* anim = new QPropertyAnimation(shadow_, "blurRadius", this);
        anim->setDuration(200);
        anim->setStartValue(shadow_->blurRadius());
        anim->setEndValue(blur);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        
        shadow_->setColor(color);
        shadow_->setOffset(offset);
    }

    QGraphicsDropShadowEffect* shadow_;
};

// ─────────────────────────────────────────────────────────────
//  MovieCard — Cinematic card with poster, gradient overlay, hover reveal
// ─────────────────────────────────────────────────────────────
class MovieCard : public HoverFrame
{
    Q_OBJECT
public:
    explicit MovieCard(const QVariantMap& item, QWidget* parent = nullptr)
        : HoverFrame(parent), item_(item)
    {
        setFixedSize(200, 320);
        setObjectName("movieCard");
        
        applyStyle(false);

        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        // Poster container with gradient overlay
        auto* posterContainer = new QWidget(this);
        posterContainer->setFixedSize(200, 260);
        posterContainer->setObjectName("posterContainer");
        
        auto* posterLay = new QVBoxLayout(posterContainer);
        posterLay->setContentsMargins(0, 0, 0, 0);
        posterLay->setSpacing(0);

        poster_ = new QLabel(posterContainer);
        poster_->setFixedSize(200, 260);
        poster_->setStyleSheet(R"(
            QLabel {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 #1a1a2e, stop:1 #252538);
                border-radius: 12px;
            }
        )");
        
        // Gradient overlay for text readability
        auto* overlay = new QWidget(poster_);
        overlay->setStyleSheet(R"(
            QWidget {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 rgba(10,10,15,0), stop:1 rgba(10,10,15,180));
                border-radius: 12px;
            }
        )");
        overlay->setFixedSize(200, 260);
        
        auto* overlayLay = new QVBoxLayout(overlay);
        overlayLay->setContentsMargins(12, 0, 12, 12);
        overlayLay->addStretch();

        // Hover button (hidden by default)
        infoBtn_ = new QPushButton("View Info", overlay);
        infoBtn_->setFixedSize(120, 36);
        infoBtn_->setCursor(Qt::PointingHandCursor);
        infoBtn_->setStyleSheet(R"(
            QPushButton {
                background: rgba(124, 58, 237, 230);
                color: white;
                font-weight: 600;
                font-size: 13px;
                border: none;
                border-radius: 18px;
            }
            QPushButton:hover {
                background: rgba(139, 92, 246, 255);
            }
        )");
        infoBtn_->hide();
        overlayLay->addWidget(infoBtn_, 0, Qt::AlignCenter);

        posterLay->addWidget(poster_);
        lay->addWidget(posterContainer);

        // Info section
        auto* infoWidget = new QWidget(this);
        infoWidget->setStyleSheet("background: transparent;");
        auto* infoLay = new QVBoxLayout(infoWidget);
        infoLay->setContentsMargins(12, 10, 12, 10);
        infoLay->setSpacing(4);

        auto* title = new QLabel(item.value("title").toString(), infoWidget);
        title->setStyleSheet(R"(
            QLabel {
                color: #f4f4f5;
                font-size: 14px;
                font-weight: 600;
                background: transparent;
            }
        )");
        title->setWordWrap(false);
        infoLay->addWidget(title);

        // Meta row
        auto* metaWidget = new QWidget(infoWidget);
        metaWidget->setStyleSheet("background: transparent;");
        auto* metaLay = new QHBoxLayout(metaWidget);
        metaLay->setContentsMargins(0, 0, 0, 0);
        metaLay->setSpacing(8);

        const double rating = item.value("rating").toDouble();
        const int year = item.value("year").toInt();
        
        if (rating > 0) {
            auto* ratingLbl = new QLabel(QString::fromUtf8("★ %1").arg(rating, 0, 'f', 1), metaWidget);
            ratingLbl->setStyleSheet("color: #fbbf24; font-size: 12px; font-weight: 700; background: transparent;");
            metaLay->addWidget(ratingLbl);
        }
        
        if (year > 0) {
            auto* yearLbl = new QLabel(QString::number(year), metaWidget);
            yearLbl->setStyleSheet("color: #a1a1aa; font-size: 12px; background: transparent;");
            metaLay->addWidget(yearLbl);
        }
        
        metaLay->addStretch();
        infoLay->addWidget(metaWidget);
        lay->addWidget(infoWidget);

        // Wire signals
            connect(infoBtn_, &QPushButton::clicked, this, [this]() {
                emit viewInfo(item_);
            });

        // Load image async
        const QString url = item.value("posterUrl").toString();
        if (!url.isEmpty()) {
            ImageLoader::instance()->load(url, poster_);
        }
    }

    QSize sizeHint() const override { return {200, 320}; }

signals:
    void viewInfo(const QVariantMap& info);

protected:
    void enterEvent(QEnterEvent* e) override
    {
        if (item_.value("hasInfo").toBool()) {
            infoBtn_->show();
            // Animate button in
            auto* anim = new QPropertyAnimation(infoBtn_, "pos", this);
            anim->setDuration(200);
            anim->setStartValue(infoBtn_->pos() + QPoint(0, 10));
            anim->setEndValue(infoBtn_->pos());
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
        applyStyle(true);
        HoverFrame::enterEvent(e);
    }

    void leaveEvent(QEvent* e) override
    {
        infoBtn_->hide();
        applyStyle(false);
        HoverFrame::leaveEvent(e);
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if (e->button() == Qt::LeftButton && item_.value("hasInfo").toBool()) {
            emit viewInfo(item_);
        }
        HoverFrame::mousePressEvent(e);
    }

private:
    void applyStyle(bool hovered)
    {
        if (hovered) {
            setStyleSheet(R"(
                QFrame#movieCard {
                    background: #1e1e2e;
                    border: 1px solid rgba(139, 92, 246, 0.3);
                    border-radius: 16px;
                }
            )");
        } else {
            setStyleSheet(R"(
                QFrame#movieCard {
                    background: #181822;
                    border: 1px solid rgba(255, 255, 255, 0.06);
                    border-radius: 16px;
                }
            )");
        }
    }

    QVariantMap item_;
    QLabel* poster_ = nullptr;
    QPushButton* infoBtn_ = nullptr;
};

// ─────────────────────────────────────────────────────────────
//  HomeWidget Implementation
// ─────────────────────────────────────────────────────────────
HomeWidget::HomeWidget(HomepageBridge* bridge, QWidget* parent)
    : QWidget(parent), bridge_(bridge)
{
    setupUi();
    
    if (bridge_) {
        connect(bridge_, &HomepageBridge::homepageReady, this, &HomeWidget::onHomepageReady);
        connect(bridge_, &HomepageBridge::homepageError, this, &HomeWidget::onHomepageError);
        connect(bridge_, &HomepageBridge::infoReady, this, &HomeWidget::onInfoReady);
        bridge_->loadHomepage();
    }
}

HomeWidget::~HomeWidget() = default;

void HomeWidget::setupUi()
{
    setObjectName("HomeWidget");
    setStyleSheet(R"(
        QWidget#HomeWidget {
            background-color: #0a0a0f;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollBar:vertical {
            width: 8px;
            background: #0a0a0f;
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

    scroll_ = new QScrollArea(this);
    scroll_->setWidgetResizable(true);
    scroll_->setFrameShape(QFrame::NoFrame);
    scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    root->addWidget(scroll_);

    content_ = new QWidget(scroll_);
    content_->setObjectName("homeContent");
    content_->setStyleSheet("background: #0a0a0f;");
    scroll_->setWidget(content_);

    auto* main = new QVBoxLayout(content_);
    main->setContentsMargins(0, 0, 0, 40);
    main->setSpacing(0);

    // ── SECTION HEADER ─────────────────────────────────────
    auto* sectionHeader = new QWidget(content_);
    sectionHeader->setStyleSheet("background: transparent;");
    auto* shLay = new QHBoxLayout(sectionHeader);
    shLay->setContentsMargins(48, 40, 48, 24);
    shLay->setSpacing(12);

    // Accent bar
    auto* accentBar = new QWidget(sectionHeader);
    accentBar->setFixedSize(4, 24);
    accentBar->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #8b5cf6, stop:1 #6d28d9);
            border-radius: 2px;
        }
    )");
    shLay->addWidget(accentBar);

    sectionTitle_ = new QLabel("Trending Now", sectionHeader);
    sectionTitle_->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 24px;
            font-weight: 700;
            background: transparent;
        }
    )");
    shLay->addWidget(sectionTitle_);
    shLay->addStretch();

    statusLabel_ = new QLabel(sectionHeader);
    statusLabel_->setStyleSheet("color: #fbbf24; font-size: 13px; background: transparent;");
    shLay->addWidget(statusLabel_);

    main->addWidget(sectionHeader);

    // ── RESPONSIVE GRID ────────────────────────────────────
    auto* gridContainer = new QWidget(content_);
    gridContainer->setStyleSheet("background: transparent;");
    grid_ = new QGridLayout(gridContainer);
    grid_->setContentsMargins(48, 0, 48, 0);
    grid_->setSpacing(20);
    
    main->addWidget(gridContainer);
    main->addStretch();
}

int HomeWidget::calculateColumns() const
{
    int width = scroll_->viewport()->width();
    if (width < 480) return 2;
    if (width < 768) return 3;
    if (width < 1200) return 4;
    if (width < 1600) return 5;
    return 6;
}

void HomeWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!currentItems_.isEmpty()) {
        QTimer::singleShot(50, this, &HomeWidget::rebuildGrid);
    }
}

void HomeWidget::rebuildGrid()
{
    clearGrid();
    
    int cols = calculateColumns();
    int row = 0, col = 0;
    
    for (const QVariant& v : currentItems_) {
        auto* card = new MovieCard(v.toMap(), this);
        connect(card, &MovieCard::viewInfo, this, [this](const QVariantMap& info) {
            if (bridge_) {
                bridge_->loadInfo(info.value("id").toString(),
                                  info.value("sourceName").toString(),
                                  info.value("type").toString());
            }
            emit infoRequested(info);
        });
        grid_->addWidget(card, row, col);
        if (++col >= cols) { col = 0; ++row; }
    }
}

void HomeWidget::clearGrid()
{
    while (QLayoutItem* item = grid_->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }
}

void HomeWidget::onHomepageReady(const QVariantList& items, const QString& sourceName, bool isFallback)
{
    currentItems_ = items;
    currentSource_ = sourceName;
    isFallback_ = isFallback;
    
    statusLabel_->setText(isFallback ? "Showing fallback data" : "");
    sectionTitle_->setText(sourceName.isEmpty() ? "Trending Now" : sourceName + " — Trending");

    rebuildGrid();
}

void HomeWidget::onInfoReady(const QVariantMap& info)
{
    emit infoRequested(info);
}

void HomeWidget::onHomepageError(const QString& message)
{
    statusLabel_->setText("Error: " + message);
    statusLabel_->setStyleSheet("color: #ef4444; font-size: 13px; background: transparent;");
}

#include "HomeWidget.moc"