#include "movie_source1/movie_source1.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>

// ── Plugin guide reference ────────────────────────────────────────────────
// Apibay is a hasInfo: FALSE torrent source. It implements only search()
// and getStreamUrl() — everything else is stubbed in the header.
//
// Search flow:
//   1. search() hits apibay.org/q.php — returns raw torrent JSON
//   2. Results go to SearchBridge which runs TitleCleaner + TMDB matching
//   3. User taps a result — InfoScreen shows with Stream/Download buttons
//   4. getStreamUrl() is called — returns a magnet URI
//   5. QueueBridge sees streamType="torrent" — routes to torrent_service
//   6. torrent_service does sequential download — fires readyToPlay at 5%
//   7. mpv opens the file and starts playing while download continues
//
// Plugin guide part 2: "stream while downloading (torrents)"
// ─────────────────────────────────────────────────────────────────────────

using json = nlohmann::json;

namespace movie_source1 {

// ── HTTP helpers ─────────────────────────────────────────────────────────

static size_t writeToString(void* c, size_t s, size_t n, void* out) {
    static_cast<std::string*>(out)->append((char*)c, s * n);
    return s * n;
}

static std::string httpGet(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string body;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "nothingmovies/1.0");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return body;
}

static std::string urlEncode(const std::string& value) {
    CURL* curl = curl_easy_init();
    char* out = curl_easy_escape(curl, value.c_str(), (int)value.size());
    std::string result(out);
    curl_free(out);
    curl_easy_cleanup(curl);
    return result;
}

// ── Capabilities ─────────────────────────────────────────────────────────
// Returns the hardcoded capability profile for Apibay.
// No info.json — source1 is built-in and its capabilities are known at
// compile time. Plugin guide part 1: "getCapabilities() — called once at startup."

core::SourceCapabilities ApibayProvider::getCapabilities() const {
    // info.json is the single source of truth for what this source can do.
    // Plugin guide part 1: "info.json — the manifest."
    // The file is copied into the build directory by CMake (see CMakeLists.txt).
    std::ifstream infoFile("movie_source1/data/info.json");
    core::SourceCapabilities caps;
    if (!infoFile) return caps; // safe defaults (all false) if file missing

    nlohmann::json j;
    infoFile >> j;
    caps.hasHomepage    = j.value("hasHomepage",  false);
    caps.hasPoster      = j.value("hasPoster",    false);
    caps.hasRating      = j.value("hasRating",    false);
    caps.hasSubtitles   = j.value("hasSubtitles", false);
    caps.hasDownload    = j.value("hasDownload",  false);
    caps.hasStream      = j.value("hasStream",    false);
    caps.hasInfo        = j.value("hasInfo",      false);
    caps.streamType     = j.value("streamType",   "");
    caps.downloadType   = j.value("downloadType", "");
    caps.version        = j.value("version",      "");
    caps.profilePicture = j.value("profilePicture", "");
    return caps;
}
// ── Quality detection ─────────────────────────────────────────────────────
// Scans the raw torrent name for common quality markers.
// Plugin guide part 2: "for hasInfo:false sources, title is the raw torrent name."

std::string ApibayProvider::detectQuality(const std::string& name) const {
    std::string n = name;
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);

    if (n.find("2160p") != std::string::npos || n.find("4k") != std::string::npos) return "4K";
    if (n.find("1080p") != std::string::npos) return "1080p";
    if (n.find("720p")  != std::string::npos) return "720p";
    if (n.find("480p")  != std::string::npos) return "480p";
    if (n.find("webrip") != std::string::npos || n.find("web-dl") != std::string::npos) return "WEB-Rip";
    return "Unknown";
}

// ── Magnet builder ────────────────────────────────────────────────────────
// Builds a full magnet URI from an info hash and display name.
// The app's QueueBridge sees the magnet: prefix and routes to torrent_service.
// Plugin guide part 2: "getStreamUrl — return magnet or HTTP URL."

std::string ApibayProvider::buildMagnet(const std::string& infoHash,
                                         const std::string& displayName) const {
    std::ostringstream magnet;
    magnet << "magnet:?xt=urn:btih:" << infoHash << "&dn=" << urlEncode(displayName);
    for (const auto& tr : trackers_) {
        magnet << "&tr=" << urlEncode(tr);
    }
    return magnet.str();
}

long long ApibayProvider::parseSize(const std::string& sizeStr) const {
    try { return std::stoll(sizeStr); } catch (...) { return 0; }
}

// ── search() ─────────────────────────────────────────────────────────────
// Hits apibay.org and returns raw torrent results. SearchBridge will clean
// titles and enrich with TMDB posters automatically (hasInfo: false path).
// Filters: seeders < 1 (dead torrent) and size > 8GB (probably not a movie).
// Plugin guide part 2: "what search() returns — for hasInfo:false sources."

std::vector<core::MediaResult> ApibayProvider::search(const std::string& query) {
    std::vector<core::MediaResult> results;

    std::string url = "https://apibay.org/q.php?q=" + urlEncode(query);
    std::string body = httpGet(url);

    json torrents;
    try {
        torrents = json::parse(body);
    } catch (...) {
        return results; // empty on parse failure — don't crash the aggregator
    }

    for (auto& t : torrents) {
        std::string name     = t.value("name", "");
        std::string infoHash = t.value("info_hash", "");
        std::string sizeStr  = t.value("size", "0");

        int seeders = 0;
        try { seeders = std::stoi(t.value("seeders", "0")); } catch (...) {}

        if (seeders < 1) continue;                      // skip dead torrents
        if (parseSize(sizeStr) > kMaxSizeBytes) continue; // skip > 8GB

        core::MediaResult r;
        r.id         = infoHash;  // info_hash is our stable id for getStreamUrl()
        r.title      = name + " [" + detectQuality(name) + "]"; // raw name for TitleCleaner
        r.posterUrl  = "";        // empty — SearchBridge fetches from TMDB
        r.sourceName = "Apibay (Torrent)"; // must match registerSource() name in main.cpp
        results.push_back(r);
    }

    return results;
}

// ── getStreamUrl() ────────────────────────────────────────────────────────
// Called when the user taps Stream or Download. The id is the info_hash
// captured during search(). Re-fetches the torrent name from Apibay so
// the magnet has a clean dn= parameter, then builds the full magnet URI.
//
// The returned magnet: URI goes to QueueBridge which sees streamType="torrent"
// and routes to torrent_service for sequential download + stream.
// Plugin guide part 2: "getStreamUrl — returning a playable URL."

std::string ApibayProvider::getStreamUrl(const std::string& id) {
    std::string url  = "https://apibay.org/t.php?info_hash=" + id;
    std::string body = httpGet(url);

    json data;
    try {
        data = json::parse(body);
    } catch (...) {
        return "";
    }

    std::string name = data.value("name", "Unknown Torrent");
    return buildMagnet(id, name);
}

} // namespace movie_source1