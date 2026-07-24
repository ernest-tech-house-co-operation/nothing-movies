#pragma once
#include <string>
#include <vector>
#include <memory>

namespace queue_manager {

enum class QueueItemType {
    Torrent,
    HttpDownload
};

enum class QueueItemState {
    Pending,
    Active,
    Paused,
    Finished,
    Error
};

struct QueueItem {
    std::string id;             // torrent info-hash, or downloader's "dl_N" id
    std::string title;          // display name, from the MediaResult that started it
    QueueItemType type = QueueItemType::HttpDownload;
    QueueItemState state = QueueItemState::Pending;
    double progress = 0.0;      // 0.0 - 1.0
    std::string filePath;       // where the finished/in-progress file lives
    bool readyToPlay = false;   // torrents: enough buffered near the front to open in a player.
                                 // Always true for HttpDownload once filePath is set.
};

// The single place that owns "where do downloads go" and routes each item
// to torrent_service or downloader depending on what it is. Nothing above
// this (UI, search_aggregator) needs to know those two modules exist.
class Queue_managerModule {
public:
    Queue_managerModule();
    ~Queue_managerModule();

    void init();

    // Default: "./downloads/nothingmoviesdownloads". Creates the folder
    // if it doesn't exist. Existing/in-progress items are not moved.
    void setDownloadFolder(const std::string& path);
    std::string getDownloadFolder() const;

    // magnetUri typically comes straight from search_aggregator::getStreamUrl().
    std::string enqueueTorrent(const std::string& title, const std::string& magnetUri);

    // url is a direct HTTP file link (non-torrent sources).
    std::string enqueueHttpDownload(const std::string& title, const std::string& url);

    // Pumps both torrent_service and downloader under the hood. Call
    // periodically (UI timer).
    void update();

    std::vector<QueueItem> listItems() const;
    QueueItem getItem(const std::string& id) const;

    void pause(const std::string& id);   // torrent-only for now, see .cpp note
    void resume(const std::string& id);  // torrent-only for now, see .cpp note
    void remove(const std::string& id);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace queue_manager