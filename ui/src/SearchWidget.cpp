#include "ui/SearchWidget.h"
#include "ui/Theme.h"
#include "ui/ImageLoader.h"
#include "ui/SearchBridge.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QFrame>


QWidget* buildResultRow(const QVariantMap& m, QWidget* parent) {
    auto* row = new QWidget(parent);
    row->setStyleSheet("background: transparent;");

    auto* lay = new QHBoxLayout(row);
    lay->setContentsMargins(14, 8, 14, 8);
    lay->setSpacing(16);

    // accent stripe
    auto* stripe = new QFrame(row);
    stripe->setFixedWidth(4);
    stripe->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
                          " stop:0 #7c3aed, stop:1 #06b6d4); border-radius: 2px;");
    lay->addWidget(stripe);

    // poster
    auto* poster = new QLabel(row);
    poster->setFixedSize(60, 80);
    poster->setStyleSheet("background-color: #2a2a3a; border-radius: 6px;");
    lay->addWidget(poster);
    const QString posterUrl = m.value("posterUrl").toString();
    if (!posterUrl.isEmpty()) ImageLoader::instance()->load(posterUrl, poster);

    // text column
    auto* col = new QVBoxLayout();
    col->setSpacing(4);

    auto* title = new QLabel(m.value("title").toString(), row);
    title->setStyleSheet("color: white; font-size: 15px; font-weight: bold;"
                         " background: transparent;");
    col->addWidget(title);

    QString meta;
    const int year = m.value("year").toInt();
    if (year > 0) meta = QString::number(year);
    const QString src = m.value("sourceName").toString();
    if (!src.isEmpty())
        meta += (meta.isEmpty() ? "" : "  ·  ") + src;

    auto* metaLbl = new QLabel(meta, row);
    metaLbl->setStyleSheet(
        QString("color: %1; font-size: 12px; background: transparent;").arg(NM_SUBTEXT));
    col->addWidget(metaLbl);
    col->addStretch();

    lay->addLayout(col, 1);
    return row;
}



SearchWidget::SearchWidget(SearchBridge* search, HomepageBridge* homepage,
                           QWidget* parent)
    : QWidget(parent), search_(search), homepage_(homepage)
{
    setObjectName("SearchWidget");
    setStyleSheet(QString("QWidget#SearchWidget { background-color: #0d0d0d; }"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── top bar ──────────────────────────────────────────────
    auto* topBar = new QWidget(this);
    topBar->setFixedHeight(64);
    topBar->setObjectName("searchTopBar");
    topBar->setStyleSheet(
        "QWidget#searchTopBar { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #1a1030, stop:1 #0f1230); border-bottom: 1px solid #2a2450; }");

    auto* tl = new QHBoxLayout(topBar);
    tl->setContentsMargins(24, 0, 24, 0);
    tl->setSpacing(12);

    query_ = new QLineEdit(topBar);
    query_->setPlaceholderText("Search movies, series...");
    query_->setStyleSheet(
        "QLineEdit { background-color: #13131f; color: white; border: 1px solid #2a2a3e;"
        "  border-radius: 8px; padding: 8px 12px; font-size: 14px; }"
        "QLineEdit:focus { border: 1px solid #7c3aed; }");
    query_->setMinimumHeight(38);
    tl->addWidget(query_, 1);

    goBtn_ = new QPushButton("Search", topBar);
    goBtn_->setFixedSize(90, 38);
    goBtn_->setCursor(Qt::PointingHandCursor);
    goBtn_->setStyleSheet(
        "QPushButton { background-color: #7c3aed; color: white; font-weight: bold;"
        "  border: none; border-radius: 8px; font-size: 14px; }"
        "QPushButton:hover { background-color: #6d28d9; }");
    tl->addWidget(goBtn_);

    movieBtn_ = new QPushButton("Movies", topBar);
    movieBtn_->setCheckable(true);
    movieBtn_->setChecked(true);
    movieBtn_->setFixedSize(80, 38);
    movieBtn_->setStyleSheet(
        "QPushButton:checked { background-color: #7c3aed; color: white; border: none; border-radius: 8px; }"
        "QPushButton:!checked { background-color: #1a1a2e; color: white; border: 1px solid #2a2a3e; border-radius: 8px; }");
    tl->addWidget(movieBtn_);

    tvBtn_ = new QPushButton("TV", topBar);
    tvBtn_->setCheckable(true);
    tvBtn_->setChecked(false);
    tvBtn_->setFixedSize(60, 38);
    tvBtn_->setStyleSheet(
        "QPushButton:checked { background-color: #7c3aed; color: white; border: none; border-radius: 8px; }"
        "QPushButton:!checked { background-color: #1a1a2e; color: white; border: 1px solid #2a2a3e; border-radius: 8px; }");
    tl->addWidget(tvBtn_);

    connect(movieBtn_, &QPushButton::clicked, this, [this]() {
        searchingTV_ = false;
        movieBtn_->setChecked(true);
        tvBtn_->setChecked(false);
    });
    connect(tvBtn_, &QPushButton::clicked, this, [this]() {
        searchingTV_ = true;
        tvBtn_->setChecked(true);
        movieBtn_->setChecked(false);
    });

    root->addWidget(topBar);

    // ── status ───────────────────────────────────────────────
    status_ = new QLabel(this);
    status_->setStyleSheet(
        QString("color: %1; font-size: 13px; padding: 12px 24px 4px 24px;").arg(NM_SUBTEXT));
    root->addWidget(status_);

    // ── results ──────────────────────────────────────────────
    list_ = new QListWidget(this);
    list_->setFrameShape(QFrame::NoFrame);
    list_->setStyleSheet(
        "QListWidget { background-color: #0d0d0d; border: none; outline: none; }"
        "QListWidget::item { background-color: #171225; border-radius: 10px;"
        "  margin: 4px 16px; }"
        "QListWidget::item:hover { background-color: #211a3d; }"
        "QListWidget::item:selected { background-color: #211a3d; }");
    list_->setSpacing(2);
    root->addWidget(list_, 1);

    connect(goBtn_, &QPushButton::clicked, this, &SearchWidget::doSearch);
    connect(query_, &QLineEdit::returnPressed, this, &SearchWidget::doSearch);
    connect(list_, &QListWidget::itemClicked,
            this, &SearchWidget::onItemClicked);

    if (search_) {
        connect(search_, &SearchBridge::resultsReady,
                this, &SearchWidget::onResultsReady);
        connect(search_, &SearchBridge::searchError,
                this, &SearchWidget::onSearchError);
    }
}

void SearchWidget::doSearch() {
    if (!search_) return;
    const QString q = query_->text().trimmed();
    if (q.isEmpty()) return;
    list_->clear();
    status_->setText("Searching...");
    if (searchingTV_)
        search_->searchTV(q);
    else
        search_->search(q);
}

void SearchWidget::onResultsReady(const QVariantList& results) {
    list_->clear();
    for (const QVariant& v : results) {
        const QVariantMap m = v.toMap();
        auto* item = new QListWidgetItem(list_);
        item->setSizeHint(QSize(0, 96));
        item->setData(Qt::UserRole, m);
        list_->setItemWidget(item, buildResultRow(m, list_));
    }
    status_->setText(results.isEmpty()
                         ? "No results."
                         : QString("%1 result(s)").arg(results.size()));
}

void SearchWidget::onSearchError(const QString& message) {
    status_->setText("Search error: " + message);
}

void SearchWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    emit resultSelected(item->data(Qt::UserRole).toMap());
}