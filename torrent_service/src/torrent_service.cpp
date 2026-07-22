#include "torrent_service/torrent_service.h"

#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_flags.hpp>

#include <iostream>
#include <unordered_map>
#include <algorithm>

namespace torrent_service {

namespace {

std::string toHex(const std::string& input) {
    static const char* hexChars = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char c : input) {
        out.push_back(hexChars[c >> 4]);
        out.push_back(hexChars[c & 0x0F]);
    }
    return out;
}

TorrentState mapState(lt::torrent_status::state_t s) {
    switch (s) {
        case lt::torrent_status::checking_files:
        case lt::torrent_status::checking_resume_data:
            return TorrentState::Checking;
        case lt::torrent_status::downloading_metadata:
        case lt::torrent_status::downloading:
            return TorrentState::Downloading;
        case lt::torrent_status::seeding:
        case lt::torrent_status::finished:
            return TorrentState::Finished;
        default:
            return TorrentState::Downloading;
    }
}

// Movies/shows are almost always one big video file plus junk (sample clips,
// nfo, subs). We only care about the largest file in the torrent.
std::string largestFilePath(const lt::torrent_handle& handle) {
    if (!handle.is_valid() || !handle.torrent_file()) return "";

    auto tf = handle.torrent_file();
    const auto& files = tf->files();

    int biggestIndex = -1;
    std::int64_t biggestSize = -1;
    for (int i = 0; i < files.num_files(); ++i) {
        const std::int64_t sz = files.file_size(i);
        if (sz > biggestSize) {
            biggestSize = sz;
            biggestIndex = i;
        }
    }
    if (biggestIndex < 0) return "";

    lt::torrent_status st = handle.status();
    return (st.save_path + "/" + files.file_path(biggestIndex));
}

} // namespace

struct Torrent_serviceModule::Impl {
    lt::session session;
    std::unordered_map<std::string, lt::torrent_handle> handles; // id (infoHash) -> handle
};

Torrent_serviceModule::Torrent_serviceModule() : impl_(std::make_unique<Impl>()) {}
Torrent_serviceModule::~Torrent_serviceModule() = default;

void Torrent_serviceModule::init() {
    lt::settings_pack settings;
    settings.set_int(lt::settings_pack::alert_mask,
                      lt::alert::status_notification | lt::alert::error_notification);
    impl_->session.apply_settings(settings);
    std::cout << "[torrent_service] initialized" << std::endl;
}

std::string Torrent_serviceModule::addMagnet(const std::string& magnetUri,
                                              const std::string& savePath) {
    lt::error_code ec;
    lt::add_torrent_params params = lt::parse_magnet_uri(magnetUri, ec);
    if (ec) {
        std::cerr << "[torrent_service] bad magnet: " << ec.message() << std::endl;
        return "";
    }

    params.save_path = savePath;
    // Sequential download so the front of the file fills in first -- needed
    // for "play while downloading" instead of waiting for 100%.
    params.flags |= lt::torrent_flags::sequential_download;

    lt::torrent_handle handle = impl_->session.add_torrent(params, ec);
    if (ec || !handle.is_valid()) {
        std::cerr << "[torrent_service] add_torrent failed: " << ec.message() << std::endl;
        return "";
    }

    const std::string id = toHex(params.info_hashes.get_best().to_string());
    impl_->handles[id] = handle;
    return id;
}

void Torrent_serviceModule::update() {
    std::vector<lt::alert*> alerts;
    impl_->session.pop_alerts(&alerts);

    for (lt::alert* a : alerts) {
        if (auto* err = lt::alert_cast<lt::torrent_error_alert>(a)) {
            std::cerr << "[torrent_service] torrent error: " << err->message() << std::endl;
        }
        // Extend here for finished/metadata alerts if you need push-style
        // notifications instead of polling getStatus()/listTorrents().
    }
}

TorrentStatus Torrent_serviceModule::getStatus(const std::string& id) const {
    TorrentStatus out;
    out.id = id;

    auto it = impl_->handles.find(id);
    if (it == impl_->handles.end() || !it->second.is_valid()) {
        out.state = TorrentState::Error;
        return out;
    }

    const lt::torrent_handle& handle = it->second;
    lt::torrent_status st = handle.status();

    out.name = st.name;
    out.state = mapState(st.state);
    out.progress = st.progress;
    out.downloadRateKBs = st.download_rate / 1000;
    out.uploadRateKBs = st.upload_rate / 1000;
    out.numPeers = st.num_peers;

    if (st.has_metadata) {
        out.filePath = largestFilePath(handle);
        // Heuristic: 5% sequentially buffered is generally enough for a
        // player to open the file and start decoding without immediately
        // stalling. Not exact -- a proper implementation would check piece
        // priorities/availability near the front of the file specifically.
        out.readyToPlay = st.progress >= 0.05;
    }

    return out;
}

std::vector<TorrentStatus> Torrent_serviceModule::listTorrents() const {
    std::vector<TorrentStatus> result;
    result.reserve(impl_->handles.size());
    for (const auto& [id, handle] : impl_->handles) {
        result.push_back(getStatus(id));
    }
    return result;
}

void Torrent_serviceModule::pause(const std::string& id) {
    auto it = impl_->handles.find(id);
    if (it != impl_->handles.end()) it->second.pause();
}

void Torrent_serviceModule::resume(const std::string& id) {
    auto it = impl_->handles.find(id);
    if (it != impl_->handles.end()) it->second.resume();
}

void Torrent_serviceModule::remove(const std::string& id, bool deleteFiles) {
    auto it = impl_->handles.find(id);
    if (it == impl_->handles.end()) return;

    lt::remove_flags_t flags{};
    if (deleteFiles) flags |= lt::session::delete_files;

    impl_->session.remove_torrent(it->second, flags);
    impl_->handles.erase(it);
}

} // namespace torrent_service