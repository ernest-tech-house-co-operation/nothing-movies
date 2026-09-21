#include "movie_source3/movie_source3.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

using json = nlohmann::json;

namespace movie_source3 {

static size_t writeCallback(void* c, size_t s, size_t n, void* out) {
    static_cast<std::string*>(out)->append((char*)c, s * n);
    return s * n;
}

std::string RivestreamProvider::httpGet(const std::string& url) const {
    CURL* curl = curl_easy_init();
    std::string body;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "nothingmovies/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return body;
}

std::string RivestreamProvider::generateSecretKey(const std::string& id) const {
    auto now = std::chrono::system_clock::now();
    auto hours = std::chrono::duration_cast<std::chrono::hours>(now.time_since_epoch()).count();
    std::string input = id + std::to_string(hours);
    uint64_t hash = 5381;
    for (char c : input) hash = ((hash << 5) + hash) + (unsigned char)c;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hash;
    return oss.str();
}

core::SourceCapabilities RivestreamProvider::getCapabilities() const {
    core::SourceCapabilities caps;
    caps.hasHomepage  = false;
    caps.hasPoster    = false;
    caps.hasRating    = false;
    caps.hasSubtitles = true;
    caps.hasDownload  = false;
    caps.hasStream    = true;
    caps.hasInfo      = false;
    caps.streamType   = "http";
    caps.version      = "1.0.0";
    return caps;
}

std::vector<core::MediaResult> RivestreamProvider::search(const std::string&) {
    return {};
}

std::string RivestreamProvider::getStreamUrl(const std::string& id) {
    std::string secretKey = generateSecretKey(id);
    for (const char* service : SERVICES) {
        std::string url = std::string(RIVESTREAM_BASE)
            + "?requestID=movieVideoProvider&id=" + id
            + "&service=" + service
            + "&secretKey=" + secretKey
            + "&proxyMode=noProxy";
        std::string body = httpGet(url);
        try {
            json data = json::parse(body);
            if (data.contains("data") && data["data"].contains("sources")
                && data["data"]["sources"].is_array()
                && !data["data"]["sources"].empty()) {
                std::string streamUrl = data["data"]["sources"][0].value("url", "");
                if (!streamUrl.empty()) return streamUrl;
            }
        } catch (...) {}
    }
    return "";
}

std::vector<std::string> RivestreamProvider::getSubtitleUrls(const std::string& id) {
    std::vector<std::string> urls;
    std::string secretKey = generateSecretKey(id);
    std::string url = std::string(RIVESTREAM_BASE)
        + "?requestID=movieVideoProvider&id=" + id
        + "&service=apex&secretKey=" + secretKey
        + "&proxyMode=noProxy";
    std::string body = httpGet(url);
    try {
        json data = json::parse(body);
        if (data.contains("data") && data["data"].contains("captions")
            && data["data"]["captions"].is_array()) {
            for (const auto& cap : data["data"]["captions"])
                urls.push_back(cap.value("file", ""));
        }
    } catch (...) {}
    return urls;
}

} // namespace movie_source3
//This absolutely does not work if you want to open a pr with this file fully fixed i will be like bro how fuck did you know all of its apis this i will fully implement it when kflix breaks or the android app is fully working because right now am going through hell with it 