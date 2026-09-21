#include "ui/ShellWidget.h"
#include "ui/HomeWidget.h"
#include "ui/SearchWidget.h"
#include "ui/DownloadsWidget.h"
#include "ui/SourcesWidget.h"
#include "ui/SettingsWidget.h"
#include "ui/ThanksWidget.h"
#include "ui/InfoWidget.h"
#include "ui/HomepageBridge.h"
#include "ui/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QStackedWidget>

ShellWidget::ShellWidget(HomepageBridge* homepage,
                         SearchBridge* search,
                         AppController* app,
                         QueueBridge* queue,
                         QWidget* parent)
    : QWidget(parent), homepage_(homepage)
{
    setObjectName("ShellWidget");
    setStyleSheet(QString("QWidget#ShellWidget { background-color: %1; }").arg(NM_BG));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── top nav bar ─────────────────────────────────────────
    auto* topBar = new QWidget(this);
    topBar->setObjectName("shellTopBar");
    topBar->setFixedHeight(64);
    topBar->setStyleSheet(
        "QWidget#shellTopBar { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #1a1030, stop:1 #0f1230); border-bottom: 1px solid #2a2450; }");

    auto* tl = new QHBoxLayout(topBar);
    tl->setContentsMargins(24, 0, 24, 0);
    tl->setSpacing(8);

    auto* brand = new QLabel("Nothing Movies", topBar);
    brand->setStyleSheet(
        "color: white; font-size: 18px; font-weight: bold; background: transparent;");
    tl->addWidget(brand);
    tl->addStretch();

    struct NavEntry { const char* label; int index; };
    const NavEntry entries[] = {
        { "🏠  Home",      0 },
        { "🔍  Search",    1 },
        { "⬇  Downloads", 2 },
        { "🔌  Sources",   3 },
        { "💜  Thanks",    4 },
        { "⚙  Settings",  5 },
    };

    navGroup_ = new QButtonGroup(this);
    navGroup_->setExclusive(true);

    for (const NavEntry& e : entries) {
        auto* btn = new QPushButton(QString::fromUtf8(e.label), topBar);
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton { color: #9a94b8; background: transparent; border: none;"
            "  padding: 8px 16px; font-size: 13px; border-radius: 18px; }"
            "QPushButton:hover { color: #e5e0ff; background: #2a2140; }"
            "QPushButton:checked { color: white; background: #7c3aed; font-weight: bold; }");
        navGroup_->addButton(btn, e.index);
        tl->addWidget(btn);
    }

    root->addWidget(topBar);

    // ── screen stack ────────────────────────────────────────
    stack_ = new QStackedWidget(this);
    root->addWidget(stack_, 1);

    home_      = new HomeWidget(homepage, stack_);
    searchW_   = new SearchWidget(search, homepage, stack_);
    downloads_ = new DownloadsWidget(queue, app, stack_);
    sources_   = new SourcesWidget(homepage, stack_);
    thanks_    = new ThanksWidget(stack_);
    settings_  = new SettingsWidget(stack_);
    info_      = new InfoWidget(search, app, queue, homepage, stack_);

    stack_->addWidget(home_);       // 0
    stack_->addWidget(searchW_);    // 1
    stack_->addWidget(downloads_);  // 2
    stack_->addWidget(sources_);    // 3
    stack_->addWidget(thanks_);     // 4
    stack_->addWidget(settings_);   // 5
    infoIndex_ = stack_->addWidget(info_); // 6

    connect(navGroup_, &QButtonGroup::idClicked,
            this, &ShellWidget::onNavClicked);
    connect(home_, &HomeWidget::infoRequested,
            this, &ShellWidget::showInfo);
        connect(searchW_, &SearchWidget::resultSelected, this,
            [this](const QVariantMap& result) {
            if (homepage_) {
                homepage_->loadInfo(result.value("id").toString(),
                                        result.value("sourceName").toString(),
                                        result.value("type").toString());
            }
            });
        if (homepage_) {
        connect(homepage_, &HomepageBridge::infoReady,
            this, &ShellWidget::showInfo);
        }
    connect(info_, &InfoWidget::backRequested,
            this, &ShellWidget::closeInfo);

    if (auto* first = navGroup_->button(0)) first->setChecked(true);
    stack_->setCurrentIndex(0);
}

void ShellWidget::onNavClicked(int index) {
    if (index < 0 || index >= infoIndex_) return;
    stack_->setCurrentIndex(index);
}

void ShellWidget::showInfo(const QVariantMap& info) {
    const int cur = stack_->currentIndex();
    if (cur != infoIndex_) prevIndex_ = cur;

    info_->setInfo(info);
    stack_->setCurrentIndex(infoIndex_);
}

void ShellWidget::closeInfo() {
    const int back = (prevIndex_ >= 0 && prevIndex_ < infoIndex_) ? prevIndex_ : 0;
    stack_->setCurrentIndex(back);
    if (auto* btn = navGroup_->button(back)) btn->setChecked(true);
}

void ShellWidget::showThanks() {
    if (auto* btn = navGroup_->button(4)) btn->setChecked(true);
    stack_->setCurrentIndex(4);
}