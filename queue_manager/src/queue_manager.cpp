#include "queue_manager/queue_manager.h"
#include "torrent_service/torrent_service.h"
#include "downloader/downloader.h"

#include <filesystem>
#include <unordered_map>

namespace fs = std::filesystem;

namespace queue_manager {

namespace {

// Torrent state -> unified queue state
QueueItemState mapTorrentState(torrent_service::TorrentState s) {
    switch (s) {
        case torrent_service::TorrentState::Checking:
            return QueueItemState::Pending;
        case torrent_service::TorrentState::Downloading:
            return QueueItemState::Active;
        case torrent_service::TorrentState::Paused:
            return QueueItemState::Paused;
        case torrent_service::TorrentState::Seeding:
        case torrent_service::TorrentState::Finished:
            return QueueItemState::Finished;
        default:
            return QueueItemState::Error;
    }
}

// Download state -> unified queue state
QueueItemState mapDownloadState(downloader::DownloadState s) {
    switch (s) {
        case downloader::DownloadState::Pending:
            return QueueItemState::Pending;
        case downloader::DownloadState::Downloading:
            return QueueItemState::Active;
        case downloader::DownloadState::Finished:
            return QueueItemState::Finished;
        default:
            return QueueItemState::Error; // Error or Cancelled both surface as Error to the UI
    }
}

// Turns "Interstellar [1080p]" into "Interstellar_1080p.download" style --
// good enough for a filesystem-safe filename, not meant to be pretty.
std::string sanitizeFilename(const std::string& title) {
    std::string out;
    out.reserve(title.size());
    for (char c : title) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.') {
            out += c;
        } else if (c == ' ') {
            out += '_';
        }
        // anything else (brackets, colons, slashes) just gets dropped
    }
    return out.empty() ? "download" : out;
}

} // namespace

struct Queue_managerModule::Impl {
    torrent_service::Torrent_serviceModule torrentService;
    downloader::DownloaderModule downloaderService;

    std::string downloadFolder = "./downloads/nothingmoviesdownloads";

    struct Meta {
        QueueItemType type;
        std::string title;
    };
    std::unordered_map<std::string, Meta> items; // id -> metadata
};

Queue_managerModule::Queue_managerModule() : impl_(std::make_unique<Impl>()) {}
Queue_managerModule::~Queue_managerModule() = default;

void Queue_managerModule::init() {
    impl_->torrentService.init();
    impl_->downloaderService.init();
    fs::create_directories(impl_->downloadFolder);
}

void Queue_managerModule::setDownloadFolder(const std::string& path) {
    fs::create_directories(path);
    impl_->downloadFolder = path;
}

std::string Queue_managerModule::getDownloadFolder() const {
    return impl_->downloadFolder;
}

std::string Queue_managerModule::enqueueTorrent(const std::string& title,
                                                  const std::string& magnetUri) {
    const std::string id = impl_->torrentService.addMagnet(magnetUri, impl_->downloadFolder);
    if (id.empty()) return "";

    impl_->items[id] = {QueueItemType::Torrent, title};
    return id;
}

std::string Queue_managerModule::enqueueHttpDownload(const std::string& title,
                                                       const std::string& url) {
    const std::string destPath = impl_->downloadFolder + "/" + sanitizeFilename(title);
    const std::string id = impl_->downloaderService.startDownload(url, destPath);
    if (id.empty()) return "";

    impl_->items[id] = {QueueItemType::HttpDownload, title};
    return id;
}

void Queue_managerModule::update() {
    impl_->torrentService.update();
    impl_->downloaderService.update();
}

QueueItem Queue_managerModule::getItem(const std::string& id) const {
    QueueItem out;
    out.id = id;

    auto it = impl_->items.find(id);
    if (it == impl_->items.end()) {
        out.state = QueueItemState::Error;
        return out;
    }

    out.title = it->second.title;
    out.type = it->second.type;

    if (out.type == QueueItemType::Torrent) {
        auto status = impl_->torrentService.getStatus(id);
        out.state = mapTorrentState(status.state);
        out.progress = status.progress;
        out.filePath = status.filePath;
    } else {
        auto status = impl_->downloaderService.getStatus(id);
        out.state = mapDownloadState(status.state);
        out.progress = status.progress;
        out.filePath = status.destPath;
    }

    return out;
}

std::vector<QueueItem> Queue_managerModule::listItems() const {
    std::vector<QueueItem> result;
    result.reserve(impl_->items.size());
    for (const auto& [id, meta] : impl_->items) {
        result.push_back(getItem(id));
    }
    return result;
}

void Queue_managerModule::pause(const std::string& id) {
    auto it = impl_->items.find(id);
    if (it == impl_->items.end()) return;

    // HTTP downloads don't support pause/resume yet -- curl range-request
    // resuming is a real feature to add later, not a quick one. Torrents
    // pause natively via libtorrent.
    if (it->second.type == QueueItemType::Torrent) {
        impl_->torrentService.pause(id);
    }
}

void Queue_managerModule::resume(const std::string& id) {
    auto it = impl_->items.find(id);
    if (it == impl_->items.end()) return;

    if (it->second.type == QueueItemType::Torrent) {
        impl_->torrentService.resume(id);
    }
}

void Queue_managerModule::remove(const std::string& id) {
    auto it = impl_->items.find(id);
    if (it == impl_->items.end()) return;

    if (it->second.type == QueueItemType::Torrent) {
        impl_->torrentService.remove(id, /*deleteFiles=*/false);
    } else {
        impl_->downloaderService.cancel(id);
    }

    impl_->items.erase(it);
}

} // namespace queue_manager