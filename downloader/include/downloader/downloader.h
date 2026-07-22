#pragma once
#include <string>
#include <memory>

namespace downloader {

enum class DownloadState {
    Pending,
    Downloading,
    Finished,
    Error,
    Cancelled
};

struct DownloadStatus {
    std::string id;
    std::string url;
    std::string destPath;
    DownloadState state = DownloadState::Pending;
    double progress = 0.0;          // 0.0 - 1.0
    long long downloadedBytes = 0;
    long long totalBytes = 0;
};

// Plain HTTP/file downloads -- not torrents, that's torrent_service's job.
// Each download runs on its own worker thread so multiple direct-download
// sources (or a source without torrent support) don't block each other.
class DownloaderModule {
public:
    DownloaderModule();
    ~DownloaderModule();

    DownloaderModule(const DownloaderModule&) = delete;
    DownloaderModule& operator=(const DownloaderModule&) = delete;

    void init();

    // Starts downloading `url` straight to `destPath` (full file path,
    // parent directory must already exist -- queue_manager handles that).
    // Returns an id used to track/cancel it.
    std::string startDownload(const std::string& url, const std::string& destPath);

    // Cheap no-op-safe call for UI polling consistency with torrent_service;
    // mainly here to reap finished worker threads.
    void update();

    DownloadStatus getStatus(const std::string& id) const;
    void cancel(const std::string& id);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace downloader