#include "movie_source1/movie_source1.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

namespace movie_source1 {

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

std::string ApibayProvider::detectQuality(const std::string& name) const {
    std::string n = name;
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);

    if (n.find("2160p") != std::string::npos || n.find("4k") != std::string::npos) return "4K";
    if (n.find("1080p") != std::string::npos) return "1080p";
    if (n.find("720p") != std::string::npos) return "720p";
    if (n.find("480p") != std::string::npos) return "480p";
    if (n.find("webrip") != std::string::npos || n.find("web-dl") != std::string::npos) return "WEB-Rip";
    return "Unknown";
}

std::string ApibayProvider::buildMagnet(const std::string& infoHash, const std::string& displayName) const {
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

std::vector<core::MediaResult> ApibayProvider::search(const std::string& query) {
    std::vector<core::MediaResult> results;

    std::string url = "https://apibay.org/q.php?q=" + urlEncode(query);
    std::string body = httpGet(url);

    json torrents;
    try {
        torrents = json::parse(body);
    } catch (...) {
        return results; // empty on parse failure, don't crash the aggregator
    }

    for (auto& t : torrents) {
        std::string name = t.value("name", "");
        std::string infoHash = t.value("info_hash", "");
        std::string sizeStr = t.value("size", "0");
        int seeders = 0;
        try { seeders = std::stoi(t.value("seeders", "0")); } catch (...) {}

        if (seeders < 1) continue;
        if (parseSize(sizeStr) > kMaxSizeBytes) continue;

        core::MediaResult r;
        r.id = infoHash;
        r.title = name + " [" + detectQuality(name) + "]";
        r.posterUrl = "";
        r.sourceName = "Apibay (Torrent)";
        results.push_back(r);
    }

    // sort by nothing yet fetched (seeders not stored in MediaResult) —
    // if you want seeder-based ranking in the aggregator, extend
    // core::MediaResult with a `seeders` field.
    return results;
}

std::string ApibayProvider::getStreamUrl(const std::string& id) {
    // `id` here is the info_hash captured during search(); we need the
    // display name too for a clean magnet dn= — simplest fix is to store
    // name alongside id in MediaResult, or re-fetch via /t.php?info_hash=
    std::string url = "https://apibay.org/t.php?info_hash=" + id;
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

}