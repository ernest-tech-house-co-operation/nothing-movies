#pragma once
#include <string>
#include <vector>
#include <memory>

namespace torrent_service {

enum class TorrentState {
    Checking,
    Downloading,
    Seeding,
    Paused,
    Finished,
    Error
};

struct TorrentStatus {
    std::string id;                                  // info-hash hex string, used as the handle key
    std::string name;
    TorrentState state = TorrentState::Checking;
    double progress = 0.0;                            // 0.0 - 1.0
    int downloadRateKBs = 0;
    int uploadRateKBs = 0;
    int numPeers = 0;
    std::string filePath;                             // populated once metadata resolves
    bool readyToPlay = false;                         // enough buffered for the player to open it
};

// Wraps a libtorrent session. Everything network/IO related happens inside
// the .cpp — this header stays libtorrent-free so other modules never need
// to know it exists.
class Torrent_serviceModule {
public:
    Torrent_serviceModule();
    ~Torrent_serviceModule();

    Torrent_serviceModule(const Torrent_serviceModule&) = delete;
    Torrent_serviceModule& operator=(const Torrent_serviceModule&) = delete;

    void init();

    // Adds a magnet URI (typically straight from movie_source1::getStreamUrl)
    // and starts fetching metadata + downloading into savePath.
    // Returns the torrent's info-hash hex string, used as its id everywhere else.
    std::string addMagnet(const std::string& magnetUri, const std::string& savePath);

    // Call this periodically (UI-side timer, e.g. every 500ms-1s) to pump
    // libtorrent's internal alert queue. Nothing above updates without it.
    void update();

    std::vector<TorrentStatus> listTorrents() const;
    TorrentStatus getStatus(const std::string& id) const;

    void pause(const std::string& id);
    void resume(const std::string& id);
    void remove(const std::string& id, bool deleteFiles);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace torrent_service