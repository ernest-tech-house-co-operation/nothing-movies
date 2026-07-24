#include "ui/QueueBridge.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace ui {

namespace {
// Extensions we'll show as playable downloads when found sitting in the
// download folder but not tracked by queue_manager (e.g. from a previous
// app run -- queue_manager's list is in-memory only and starts empty on
// every launch).
const QSet<QString>& videoExtensions() {
    static const QSet<QString> exts = {
        "mp4", "mkv", "avi", "webm", "mov", "m4v", "ts", "flv", "wmv"
    };
    return exts;
}
}  // namespace

QueueBridge::QueueBridge(std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                          QObject* parent)
    : QObject(parent), queueManager_(std::move(queueManager)) {
    pollTimer_ = new QTimer(this);
    connect(pollTimer_, &QTimer::timeout, this, &QueueBridge::refresh);
    pollTimer_->start(1000);
}

void QueueBridge::refresh() {
    emit itemsReady(toVariantList());
}

bool QueueBridge::enqueue(const QString& title, const QString& url) {
    const std::string titleStd = title.toStdString();
    const std::string urlStd = url.toStdString();

    const std::string id = (urlStd.rfind("magnet:", 0) == 0)
        ? queueManager_->enqueueTorrent(titleStd, urlStd)
        : queueManager_->enqueueHttpDownload(titleStd, urlStd);

    if (id.empty()) return false;

    refresh();  // let Downloads reflect the new item immediately
    return true;
}

QString QueueBridge::downloadFolder() const {
    return QString::fromStdString(queueManager_->getDownloadFolder());
}

QString QueueBridge::stateToString(queue_manager::QueueItemState state) {
    switch (state) {
        case queue_manager::QueueItemState::Pending: return "Pending";
        case queue_manager::QueueItemState::Active:  return "Active";
        case queue_manager::QueueItemState::Paused:  return "Paused";
        case queue_manager::QueueItemState::Finished: return "Finished";
        default: return "Error";
    }
}

QVariantList QueueBridge::toVariantList() const {
    QVariantList list;
    QSet<QString> knownPaths;

    for (const auto& item : queueManager_->listItems()) {
        QVariantMap m;
        m["id"] = QString::fromStdString(item.id);
        m["title"] = QString::fromStdString(item.title);
        m["state"] = stateToString(item.state);
        m["progress"] = item.progress;
        m["filePath"] = QString::fromStdString(item.filePath);
        m["readyToPlay"] = item.readyToPlay;
        list.append(m);

        if (!item.filePath.empty()) {
            knownPaths.insert(QFileInfo(QString::fromStdString(item.filePath)).canonicalFilePath());
        }
    }

    // queue_manager only knows about downloads started this session. Scan
    // the actual download folder so finished downloads from a previous run
    // still show up instead of the list looking empty every time the app
    // restarts.
    const QString folder = downloadFolder();
    QDir dir(folder);
    if (dir.exists()) {
        const auto entries = dir.entryInfoList(QDir::Files, QDir::Time);
        for (const QFileInfo& info : entries) {
            if (!videoExtensions().contains(info.suffix().toLower())) continue;
            if (knownPaths.contains(info.canonicalFilePath())) continue;

            QVariantMap m;
            m["id"] = "disk:" + info.canonicalFilePath();
            m["title"] = info.completeBaseName();
            m["state"] = "Finished";
            m["progress"] = 1.0;
            m["filePath"] = info.canonicalFilePath();
            m["readyToPlay"] = true;
            list.append(m);
        }
    }

    return list;
}

}  // namespace ui
