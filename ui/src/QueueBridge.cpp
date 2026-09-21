#include "ui/QueueBridge.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QVariantMap>


namespace {
const QSet<QString>& videoExtensions() {
    static const QSet<QString> exts = {
        "mp4", "mkv", "avi", "webm", "mov", "m4v", "ts", "flv", "wmv"
    };
    return exts;
}

const QSet<QString>& imageExtensions() {
    static const QSet<QString> exts = {
        "jpg", "jpeg", "png", "webp"
    };
    return exts;
}

// Looks for a poster/cover image sitting next to the video file (common
// with torrent releases that ship a jpg/png alongside the video in the
// same release folder). Returns a file:// URL QML can load directly, or
// an empty string if nothing suitable is found.
QString findSiblingCover(const QFileInfo& videoFile) {
    QDir sameDir = videoFile.dir();
    const auto siblings = sameDir.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo& sibling : siblings) {
        if (imageExtensions().contains(sibling.suffix().toLower())) {
            return "file://" + sibling.canonicalFilePath();
        }
    }
    return QString();
}
} // namespace

QueueBridge::QueueBridge(std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                         QObject* parent)
    : QObject(parent), queueManager_(std::move(queueManager))
{
    pollTimer_ = new QTimer(this);
    connect(pollTimer_, &QTimer::timeout, this, [this]() {
        queueManager_->update();
        // check pending streams first
        QList<PendingStream> stillPending;
        for (const auto& ps : pendingStreams_) {
            auto item = queueManager_->getItem(ps.torrentId.toStdString());
            if (item.readyToPlay && !item.filePath.empty()) {
                emit torrentReadyToPlay(ps.title, QString::fromStdString(item.filePath));
            } else if (item.state == queue_manager::QueueItemState::Error) {
                emit streamError("Torrent failed to load: " + ps.title);
            } else {
                stillPending.append(ps);
            }
        }
        pendingStreams_ = stillPending;

        // always emit the full list for DownloadsScreen
        emit itemsReady(toVariantList());
    });
    pollTimer_->start(1000);
}

void QueueBridge::refresh() {
    emit itemsReady(toVariantList());
}

bool QueueBridge::enqueue(const QString& title, const QString& url) {
    const std::string titleStd = title.toStdString();
    const std::string urlStd   = url.toStdString();

    if (urlStd.rfind("magnet:", 0) == 0) {
        startMetadataFetch(url);
        return true;
    }

    const std::string id = queueManager_->enqueueHttpDownload(titleStd, urlStd);

    if (id.empty()) return false;
    refresh();
    return true;
}

void QueueBridge::streamTorrent(const QString& title, const QString& magnetUri) {
    const std::string id = queueManager_->enqueueTorrent(
        title.toStdString(), magnetUri.toStdString());

    if (id.empty()) {
        emit streamError("Could not start torrent for: " + title);
        return;
    }

    // track it — poll loop will fire torrentReadyToPlay when buffered enough
    pendingStreams_.append({ title, QString::fromStdString(id) });
    refresh();
}

void QueueBridge::streamHttp(const QString& title, const QString& url) {
    // HTTP streams are immediate — no buffering needed
    emit httpStreamReady(title, url);
}

void QueueBridge::startMetadataFetch(const QString& magnetUri) {
    const std::string id = queueManager_->fetchMetadata(magnetUri.toStdString());
    if (id.empty()) return;

    const QString qid = QString::fromStdString(id);
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, qid]() {
        const auto files = queueManager_->getMetadata(qid.toStdString());
        if (files.empty()) return;

        timer->stop();
        timer->deleteLater();

        QVariantList list;
        for (const auto& f : files) {
            QVariantMap m;
            m["index"] = f.index;
            m["path"] = QString::fromStdString(f.path);
            m["size"] = static_cast<qlonglong>(f.size);
            list.append(m);
        }
        emit metadataReady(qid, list);
    });
    timer->start(500);
}

void QueueBridge::confirmSelection(const QString& id, const QString& title,
                                   const QList<int>& indices) {
    std::vector<int> selected(indices.begin(), indices.end());
    queueManager_->enqueueWithSelection(title.toStdString(), id.toStdString(), selected);
    refresh();
}

void QueueBridge::remove(const QString& id) {
    queueManager_->remove(id.toStdString());
    refresh();
}

QString QueueBridge::downloadFolder() const {
    return QString::fromStdString(queueManager_->getDownloadFolder());
}

QString QueueBridge::stateToString(queue_manager::QueueItemState state) {
    switch (state) {
        case queue_manager::QueueItemState::Pending:  return "Pending";
        case queue_manager::QueueItemState::Active:   return "Active";
        case queue_manager::QueueItemState::Paused:   return "Paused";
        case queue_manager::QueueItemState::Finished: return "Finished";
        default: return "Error";
    }
}

QVariantList QueueBridge::toVariantList() const {
    QVariantList list;
    QSet<QString> knownPaths;

    for (const auto& item : queueManager_->listItems()) {
        QVariantMap m;
        m["id"]          = QString::fromStdString(item.id);
        m["title"]       = QString::fromStdString(item.title);
        m["state"]       = stateToString(item.state);
        m["progress"]    = item.progress;
        m["filePath"]    = QString::fromStdString(item.filePath);
        m["readyToPlay"] = item.readyToPlay;
        m["peers"]       = item.numPeers;
        m["numSeeds"]    = item.numSeeds;
        m["downloadRate"] = item.downloadRateKBs;
        m["coverPath"]   = QString(); // live queue items don't carry a cover yet
        list.append(m);

        if (!item.filePath.empty())
            knownPaths.insert(QFileInfo(QString::fromStdString(item.filePath)).canonicalFilePath());
    }

    // Show finished downloads from previous sessions. Scans the whole
    // download folder RECURSIVELY, so releases that land inside their own
    // subfolder (e.g. "Movie Name (2021) [1080p]/movie.file.mp4", which is
    // how most torrent clients/trackers package things) are picked up too,
    // not just files sitting loose at the top level.
    const QString folder = downloadFolder();
    QDir rootDir(folder);
    if (rootDir.exists()) {
        QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            const QFileInfo info = it.fileInfo();

            if (!videoExtensions().contains(info.suffix().toLower())) continue;
            if (knownPaths.contains(info.canonicalFilePath())) continue;

            QVariantMap m;
            m["id"]          = "disk:" + info.canonicalFilePath();
            m["title"]       = info.completeBaseName();
            m["state"]       = "Finished";
            m["progress"]    = 1.0;
            m["filePath"]    = info.canonicalFilePath();
            m["readyToPlay"] = true;
            // Empty string if no image sits next to the video — QML should
            // show a blank/placeholder in that case, not a broken image.
            m["coverPath"]   = findSiblingCover(info);
            list.append(m);
        }
    }

    return list;
}

