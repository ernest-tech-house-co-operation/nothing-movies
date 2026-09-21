#include "ui/SourcesWidget.h"
#include "ui/Theme.h"
#include "ui/HomepageBridge.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QPushButton>
#include <QGridLayout>

SourcesWidget::SourcesWidget(HomepageBridge* bridge, QWidget* parent)
    : QWidget(parent), bridge_(bridge)
{
    setObjectName("SourcesWidget");
    setStyleSheet(QString("QWidget#SourcesWidget { background-color: %1; }").arg(NM_BG));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    root->addWidget(scroll);

    auto* content = new QWidget(scroll);
    content->setStyleSheet("background: transparent;");
    scroll->setWidget(content);

    auto* main = new QVBoxLayout(content);
    main->setContentsMargins(48, 48, 48, 48);
    main->setSpacing(24);

    // ── Header ────────────────────────────────────────────
    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(16);

    auto* headerText = new QVBoxLayout();
    headerText->setSpacing(6);

    auto* title = new QLabel("Content Sources", content);
    title->setStyleSheet(
        "color: white; font-size: 28px; font-weight: bold; background: transparent;");
    headerText->addWidget(title);

    auto* subtitle = new QLabel(
        "Every source compiled into this build. These are third party services — "
        "we reverse engineered most of them. They work until they don't.", content);
    subtitle->setWordWrap(true);
    subtitle->setStyleSheet(
        QString("color: %1; font-size: 13px; background: transparent;").arg(NM_SUBTEXT));
    headerText->addWidget(subtitle);
    headerRow->addLayout(headerText, 1);
    main->addLayout(headerRow);

    // ── Divider ───────────────────────────────────────────
    auto* rule = new QFrame(content);
    rule->setFixedHeight(2);
    rule->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #7c3aed, stop:1 #06b6d4); border-radius: 1px;");
    main->addWidget(rule);

    // ── Disclaimer banner ─────────────────────────────────
    auto* banner = new QFrame(content);
    banner->setStyleSheet(
        "QFrame { background: rgba(124, 58, 237, 0.08); border: 1px solid rgba(124, 58, 237, 0.25);"
        " border-radius: 10px; }");
    auto* bannerLay = new QHBoxLayout(banner);
    bannerLay->setContentsMargins(16, 12, 16, 12);
    bannerLay->setSpacing(12);

    auto* bannerIcon = new QLabel("⚠️", banner);
    bannerIcon->setStyleSheet("font-size: 20px; background: transparent;");
    bannerLay->addWidget(bannerIcon);

    auto* bannerText = new QLabel(
        "These sources are not affiliated with Nothing Movies. No agreements were made. "
        "Availability may change at any time — if something breaks, that's probably why.", banner);
    bannerText->setWordWrap(true);
    bannerText->setStyleSheet(
        "color: #c4b5fd; font-size: 12px; background: transparent;");
    bannerLay->addWidget(bannerText, 1);
    main->addWidget(banner);

    // ── Cards container ───────────────────────────────────
    cardsLayout_ = new QVBoxLayout();
    cardsLayout_->setSpacing(14);
    main->addLayout(cardsLayout_);

    main->addStretch();

    reload();
}

void SourcesWidget::reload() {
    // Clear existing cards
    while (QLayoutItem* item = cardsLayout_->takeAt(0)) {
        if (QWidget* w = item->widget()) w->deleteLater();
        delete item;
    }

    if (!bridge_) return;

    const QVariantList sources = bridge_->getSources();

    if (sources.isEmpty()) {
        auto* empty = new QLabel("No sources found.", this);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet(
            QString("color: %1; font-size: 14px; background: transparent;").arg(NM_SUBTEXT));
        cardsLayout_->addWidget(empty);
        return;
    }

    for (const QVariant& v : sources) {
        const QVariantMap m = v.toMap();

        auto* card = new QFrame(this);
        card->setStyleSheet(
            "QFrame { background: #13101f; border: 1px solid #1e1a30; border-radius: 14px; }");

        auto* cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(24, 20, 24, 20);
        cardLay->setSpacing(14);

        // ── Card header row ───────────────────────────────
        auto* topRow = new QHBoxLayout();
        topRow->setSpacing(12);

        // Accent dot
        auto* dot = new QFrame(card);
        dot->setFixedSize(10, 10);
        dot->setStyleSheet("background: #7c3aed; border-radius: 5px;");
        topRow->addWidget(dot, 0, Qt::AlignVCenter);

        // Source name
        auto* name = new QLabel(m.value("name").toString(), card);
        name->setStyleSheet(
            "color: white; font-size: 16px; font-weight: bold; background: transparent;");
        topRow->addWidget(name);

        // Version pill
        const QString version = m.value("version").toString();
        if (!version.isEmpty()) {
            auto* vPill = new QLabel("v" + version, card);
            vPill->setStyleSheet(
                "color: #a78bfa; background: rgba(124,58,237,0.15);"
                " border: 1px solid rgba(124,58,237,0.3);"
                " border-radius: 8px; padding: 2px 10px; font-size: 11px;");
            topRow->addWidget(vPill);
        }

        topRow->addStretch();

        // Stream type badge
        const QString streamType = m.value("streamType").toString();
        if (!streamType.isEmpty()) {
            auto* sBadge = new QLabel(streamType.toUpper(), card);
            sBadge->setStyleSheet(
                "color: #06b6d4; background: rgba(6,182,212,0.1);"
                " border: 1px solid rgba(6,182,212,0.3);"
                " border-radius: 8px; padding: 2px 10px; font-size: 11px; font-weight: bold;");
            topRow->addWidget(sBadge);
        }

        cardLay->addLayout(topRow);

        // ── Divider ───────────────────────────────────────
        auto* cardRule = new QFrame(card);
        cardRule->setFixedHeight(1);
        cardRule->setStyleSheet("background: #1e1a30;");
        cardLay->addWidget(cardRule);

        // ── Capabilities row ──────────────────────────────
        auto* capsRow = new QHBoxLayout();
        capsRow->setSpacing(10);

        auto mkCap = [&](const QString& label, bool enabled) {
            auto* cap = new QLabel(
                (enabled ? "✓  " : "✗  ") + label, card);
            cap->setStyleSheet(QString(
                "color: %1; background: %2; border: 1px solid %3;"
                " border-radius: 8px; padding: 4px 12px; font-size: 12px; font-weight: 600;")
                .arg(enabled ? "#4ade80" : "#6b7280")
                .arg(enabled ? "rgba(74,222,128,0.08)" : "rgba(107,114,128,0.08)")
                .arg(enabled ? "rgba(74,222,128,0.2)" : "rgba(107,114,128,0.2)"));
            return cap;
        };

        capsRow->addWidget(mkCap("Homepage",  m.value("hasHomepage").toBool()));
        capsRow->addWidget(mkCap("Info",      m.value("hasInfo").toBool()));
        capsRow->addWidget(mkCap("Streaming", m.value("hasStream").toBool()));
        capsRow->addStretch();

        cardLay->addLayout(capsRow);
        cardsLayout_->addWidget(card);
    }
}