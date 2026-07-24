#pragma once
#include <QObject>
#include <QVariantList>
#include <QTimer>
#include <memory>
#include "queue_manager/queue_manager.h"

namespace ui {

// Polls queue_manager and hands QML a flat list of {id, title, type,
// state, progress, filePath, readyToPlay} so DownloadsScreen can show
// real in-progress/finished items instead of a placeholder.
class QueueBridge : public QObject {
    Q_OBJECT
public:
    explicit QueueBridge(std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                          QObject* parent = nullptr);

    // QML can call this on becoming visible for an immediate refresh,
    // on top of the automatic poll below.
    Q_INVOKABLE void refresh();

    // Queues title/url for download without starting playback -- routes to
    // torrent_service or the plain HTTP downloader depending on the url
    // scheme. Returns false if queue_manager couldn't accept it.
    Q_INVOKABLE bool enqueue(const QString& title, const QString& url);

    // Lets the UI show the user exactly where their files land instead of
    // that being a mystery.
    Q_INVOKABLE QString downloadFolder() const;

signals:
    void itemsReady(const QVariantList& items);

private:
    std::shared_ptr<queue_manager::Queue_managerModule> queueManager_;
    QTimer* pollTimer_ = nullptr;

    static QString stateToString(queue_manager::QueueItemState state);
    QVariantList toVariantList() const;
};

}  // namespace ui
