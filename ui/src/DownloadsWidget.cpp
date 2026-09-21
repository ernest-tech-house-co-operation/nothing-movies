#include "ui/DownloadsWidget.h"
#include "ui/Theme.h"
#include "ui/ImageLoader.h"
#include "ui/QueueBridge.h"
#include "ui/AppController.h"
#include "ui/TorrentFileSelectDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QFrame>
#include <QDialog>

DownloadsWidget::DownloadsWidget(QueueBridge* queue, AppController* app,
                                 QWidget* parent)
    : QWidget(parent), queue_(queue), app_(app)
{
    setObjectName("DownloadsWidget");
    setStyleSheet(QString("QWidget#DownloadsWidget { background-color: %1; }").arg(NM_BG));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(32, 32, 32, 32);
    root->setSpacing(16);

    auto* title = new QLabel("Downloads", this);
    title->setStyleSheet(
        "color: white; font-size: 28px; font-weight: bold; background: transparent;");
    root->addWidget(title);

    const QString folder = queue_ ? queue_->downloadFolder() : QString();
    folderLabel_ = new QLabel(
        QString("Saved to %1 — downloads keep going in the background even if you "
                "leave this screen or stop watching.").arg(folder), this);
    folderLabel_->setWordWrap(true);
    folderLabel_->setStyleSheet(
        "color: #6f6a92; font-size: 11px; background: transparent;");
    root->addWidget(folderLabel_);

    emptyLabel_ = new QLabel(
        "Nothing here yet. Start a stream or download from Search.", this);
    emptyLabel_->setStyleSheet(
        "color: #8b86a8; font-size: 13px; background: transparent;");
    root->addWidget(emptyLabel_);

    list_ = new QListWidget(this);
    list_->setFrameShape(QFrame::NoFrame);
    list_->setStyleSheet(
        "QListWidget { background-color: #0b0b12; border: none; outline: none; }"
        "QListWidget::item { background-color: #171225; border-radius: 10px;"
        "  margin: 5px 0; }"
        "QListWidget::item:hover { background-color: #211a3d; }");
    root->addWidget(list_, 1);

    if (queue_) {
        connect(queue_, &QueueBridge::itemsReady,
                this, &DownloadsWidget::onItemsReady);
        connect(queue_, &QueueBridge::metadataReady,
            this, &DownloadsWidget::onMetadataReady);
        queue_->refresh();
    }
}

QWidget* DownloadsWidget::buildRow(const QVariantMap& m) {
    auto* row = new QWidget(list_);
    row->setStyleSheet("background: transparent;");

    auto* lay = new QHBoxLayout(row);
    lay->setContentsMargins(20, 10, 20, 10);
    lay->setSpacing(16);

    const bool finished = (m.value("state").toString() == "Finished");

    // accent stripe
    auto* stripe = new QFrame(row);
    stripe->setFixedWidth(4);
    stripe->setStyleSheet(QString("background-color: %1; border-radius: 2px;")
                              .arg(finished ? "#22c55e" : "#06b6d4"));
    lay->addWidget(stripe);

    // cover thumbnail
    auto* cover = new QLabel(row);
    cover->setFixedSize(48, 56);
    cover->setStyleSheet("background-color: #211a35; border-radius: 6px;");
    lay->addWidget(cover);
    const QString coverPath = m.value("coverPath").toString();
    if (!coverPath.isEmpty()) ImageLoader::instance()->load(coverPath, cover);

    // middle column
    auto* col = new QVBoxLayout();
    col->setSpacing(6);

    auto* titleLbl = new QLabel(m.value("title").toString(), row);
    titleLbl->setStyleSheet(
        "color: white; font-size: 15px; font-weight: bold; background: transparent;");
    col->addWidget(titleLbl);

    if (!finished) {
        auto* progRow = new QHBoxLayout();
        progRow->setSpacing(10);

        auto* bar = new QProgressBar(row);
        bar->setRange(0, 100);
        bar->setValue(int(m.value("progress").toDouble() * 100.0));
        bar->setTextVisible(false);
        bar->setFixedSize(160, 6);
        bar->setStyleSheet(
            "QProgressBar { background-color: #2a2a3a; border: none; border-radius: 3px; }"
            "QProgressBar::chunk { background-color: #7c3aed; border-radius: 3px; }");
        progRow->addWidget(bar);

        const int pct = qRound(m.value("progress").toDouble() * 100.0);
        auto* stateLbl = new QLabel(
            QString("%1%  ·  %2  ·  ↓ %3 KB/s  ·  👥 %4 peers")
                .arg(pct)
                .arg(m.value("state").toString())
                .arg(m.value("downloadRate", 0).toInt())
                .arg(m.value("peers", 0).toInt()),
            row);
        stateLbl->setStyleSheet(
            "color: #a1a1c9; font-size: 11px; background: transparent;");
        progRow->addWidget(stateLbl);
        progRow->addStretch();
        col->addLayout(progRow);
    } else {
        auto* ready = new QLabel("Ready to play", row);
        ready->setStyleSheet(
            "color: #4ade80; font-size: 11px; background: transparent;");
        col->addWidget(ready);
    }

    lay->addLayout(col, 1);

    // play button
    if (m.value("readyToPlay").toBool()) {
        auto* play = new QPushButton(QString::fromUtf8("▶  Play"), row);
        play->setFixedSize(84, 34);
        play->setCursor(Qt::PointingHandCursor);
        play->setStyleSheet(
            "QPushButton { background-color: #7c3aed; color: white; font-weight: bold;"
            "  border: none; border-radius: 8px; font-size: 12px; }"
            "QPushButton:hover { background-color: #8b5cf6; }");

        const QString t = m.value("title").toString();
        const QString p = m.value("filePath").toString();
        connect(play, &QPushButton::clicked, this, [this, t, p]() {
            if (app_) app_->startStream(t, p, "native");
        });
        lay->addWidget(play);
    }

    return row;
}

void DownloadsWidget::onItemsReady(const QVariantList& items) {
    list_->clear();

    emptyLabel_->setVisible(items.isEmpty());

    for (const QVariant& v : items) {
        const QVariantMap m = v.toMap();
        auto* item = new QListWidgetItem(list_);
        item->setSizeHint(QSize(0, 76));
        list_->setItemWidget(item, buildRow(m));
    }
}

void DownloadsWidget::onMetadataReady(const QString& id, const QVariantList& files) {
    std::vector<std::pair<int, std::string>> fileList;
    for (const QVariant& v : files) {
        const QVariantMap m = v.toMap();
        fileList.push_back({m.value("index").toInt(),
                            m.value("path").toString().toStdString()});
    }

    TorrentFileSelectDialog dialog(this);
    dialog.setFiles(fileList);
    if (dialog.exec() == QDialog::Accepted) {
        const auto selected = dialog.selectedIndices();
        QList<int> selectedQt;
        for (int index : selected) selectedQt.append(index);
        queue_->confirmSelection(id, "Download", selectedQt);
    } else {
        queue_->remove(id);
    }
}