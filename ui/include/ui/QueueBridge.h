#pragma once
#include <QObject>
#include <QVariantList>
#include <QList>
#include <QTimer>
#include <memory>
#include "queue_manager/queue_manager.h"



class QueueBridge : public QObject {
    Q_OBJECT
public:
    explicit QueueBridge(std::shared_ptr<queue_manager::Queue_managerModule> queueManager,
                         QObject* parent = nullptr);

    Q_INVOKABLE void refresh();

    // Download only — routes torrent vs http via magnet: prefix
    Q_INVOKABLE bool enqueue(const QString& title, const QString& url);

    // Stream a torrent — enqueues with sequential download, polls until
    // readyToPlay, then emits torrentReadyToPlay(title, filePath)
    Q_INVOKABLE void streamTorrent(const QString& title, const QString& magnetUri);

    // Stream an HTTP url — emits httpStreamReady(title, url) immediately
    Q_INVOKABLE void streamHttp(const QString& title, const QString& url);

    Q_INVOKABLE void startMetadataFetch(const QString& magnetUri);
    Q_INVOKABLE void confirmSelection(const QString& id, const QString& title,
                                      const QList<int>& indices);
    Q_INVOKABLE void remove(const QString& id);

    Q_INVOKABLE QString downloadFolder() const;

signals:
    void itemsReady(const QVariantList& items);
    void torrentReadyToPlay(const QString& title, const QString& filePath);
    void httpStreamReady(const QString& title, const QString& url);
    void streamError(const QString& message);
    void metadataReady(const QString& id, const QVariantList& files);

private:
    std::shared_ptr<queue_manager::Queue_managerModule> queueManager_;
    QTimer* pollTimer_ = nullptr;

    // tracks torrent ids that are pending stream (not download)
    struct PendingStream {
        QString title;
        QString torrentId;
    };
    QList<PendingStream> pendingStreams_;

    static QString stateToString(queue_manager::QueueItemState state);
    QVariantList toVariantList() const;
};
