#include "movie_source2/movie_source2.h"
#include "scraper_core/ScraperEngine.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <QJsonObject>
#include <QString>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

namespace movie_source2 {

namespace {
size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}
}

std::string AnimeCloudProvider::dataFilePath(const std::string& filename) const {
    return "movie_source2/data/" + filename;
}

std::string AnimeCloudProvider::httpPost(const std::string& endpoint, const std::string& jsonBody) const {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    std::string url = m_baseUrl + endpoint;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[movie_source2] curl error on " << endpoint << ": "
                   << curl_easy_strerror(res) << "\n";
        response.clear();
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

bool AnimeCloudProvider::init() {
    if (!scraper_core::isAvailable()) {
        std::cerr << "[movie_source2] Nothing Browser not found — "
                     "stream extraction will fail until it's installed\n";
        return false;
    }
    return true;
}

core::SourceCapabilities AnimeCloudProvider::getCapabilities() const {
    if (m_capabilitiesLoaded) return m_capabilities;

    std::ifstream file(dataFilePath("info.json"));
    if (!file.is_open()) {
        std::cerr << "[movie_source2] could not open info.json\n";
        return {};
    }

    json manifest;
    file >> manifest;

    core::SourceCapabilities caps;
    caps.hasHomepage    = manifest.value("hasHomepage", false);
    caps.hasPoster      = manifest.value("hasPoster", false);
    caps.hasRating      = manifest.value("hasRating", false);
    caps.hasSubtitles   = manifest.value("hasSubtitles", false);
    caps.hasDownload    = manifest.value("hasDownload", false);
    caps.hasStream      = manifest.value("hasStream", false);
    caps.hasInfo        = manifest.value("hasInfo", false);
    caps.streamType     = manifest.value("streamType", "");
    caps.downloadType   = manifest.value("downloadType", "");
    caps.version        = manifest.value("version", "");
    caps.profilePicture = manifest.value("profilePicture", "");

    if (manifest.contains("info")) {
        auto& info = manifest["info"];
        caps.info.hasMovie     = info.value("hasMovie", false);
        caps.info.hasSeries    = info.value("hasSeries", false);
        caps.info.hasYear      = info.value("hasYear", false);
        caps.info.hasRating    = info.value("hasRating", false);
        caps.info.hasCast      = info.value("hasCast", false);
        caps.info.hasSynopsis  = info.value("hasSynopsis", false);
        caps.info.hasTrailer   = info.value("hasTrailer", false);
        caps.info.hasQuality   = info.value("hasQuality", false);
        caps.info.hasLength    = info.value("hasLength", false);
        caps.info.hasSubtitles = info.value("hasSubtitles", false);
    }

    m_capabilities = caps;
    m_capabilitiesLoaded = true;
    return caps;
}

std::vector<core::HomepageItem> AnimeCloudProvider::getHomepage() {
    std::vector<core::HomepageItem> items;

    std::string raw = httpPost("/api.v1.anime.AnimeService/ListAnimesByViewCount", R"({"page": 1})");
    if (raw.empty()) return items;

    try {
        json parsed = json::parse(raw);
        for (auto& anime : parsed.value("animes", json::array())) {
            core::HomepageItem item;
            item.id       = anime.value("slug", "");
            item.title    = anime.value("title", "");
            item.imageUrl = anime.value("poster", "");
            item.genre    = anime.value("genre", "");
            item.rating   = anime.value("rating", 0.0);
            items.push_back(item);
        }
    } catch (const json::exception& e) {
        std::cerr << "[movie_source2] getHomepage parse error: " << e.what() << "\n";
    }

    return items;
}

std::vector<core::MediaResult> AnimeCloudProvider::search(const std::string& query) {
    std::vector<core::MediaResult> results;

    json body = {{"q", query}};
    std::string raw = httpPost("/api.v1.AnimeSearchService/SearchAnimes", body.dump());
    if (raw.empty()) return results;

    try {
        json parsed = json::parse(raw);
        for (auto& anime : parsed.value("animes", json::array())) {
            core::MediaResult result;
            result.id         = anime.value("slug", "");
            result.title      = anime.value("title", "");
            result.posterUrl  = anime.value("poster", "");
            result.sourceName = "AnimeCloud";
            results.push_back(result);
        }
    } catch (const json::exception& e) {
        std::cerr << "[movie_source2] search parse error: " << e.what() << "\n";
    }

    return results;
}

core::MediaInfo AnimeCloudProvider::getMediaInfo(const std::string& id) {
    core::MediaInfo info;

    json body = {{"slug", id}};
    std::string raw = httpPost("/api.v1.anime.AnimeService/GetAnime", body.dump());
    if (raw.empty()) return info;

    try {
        json anime = json::parse(raw);
        info.id          = id;
        info.title       = anime.value("title", "");
        info.synopsis    = anime.value("synopsis", "");
        info.posterUrl   = anime.value("poster", "");
        info.backdropUrl = anime.value("backdrop", "");
        info.year        = anime.value("year", 0);
        info.rating      = anime.value("rating", 0.0);
        info.type        = anime.contains("seasons") ? "series" : "movie";

        for (auto& season : anime.value("seasons", json::array())) {
            for (auto& ep : season.value("episodes", json::array())) {
                json epBody = {
                    {"slug", id},
                    {"season", season.value("number", 1)},
                    {"episode", ep.value("number", 1)}
                };
                std::string epRaw = httpPost("/api.v1.anime.AnimeService/GetEpisode", epBody.dump());
                if (epRaw.empty()) continue;

                json epData = json::parse(epRaw);
                core::MediaInfo::Episode episode;
                episode.season   = season.value("number", 0);
                episode.episode  = ep.value("number", 0);
                episode.title    = ep.value("title", "");
                episode.synopsis = ep.value("synopsis", "");
                // streamUrl here stores the raw link GetEpisode returns — the
                // app is expected to pass this value into getStreamUrl(id)
                // later to resolve it through Nothing Browser.
                episode.streamUrl = epData.value("link", "");
                info.episodes.push_back(episode);
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "[movie_source2] getMediaInfo parse error: " << e.what() << "\n";
    }

    return info;
}

std::string AnimeCloudProvider::getStreamUrl(const std::string& id) {
    scraper_core::NothingBrowser browser;
    if (!browser.start()) {
        std::cerr << "[movie_source2] failed to start Nothing Browser\n";
        return "";
    }

    QString base = QString::fromStdString(m_baseUrl);
    QString epId = QString::fromStdString(id);
    QString name = QString::fromStdString(m_siteName);

    if (!browser.registerSite(name, base)) {
        browser.shutdown();
        return "";
    }

    // site.navigate(url) — real method name from nothing-browser's site.js
    browser.call(name, "navigate", {QString("%1/proxy/player/%2").arg(base, epId)});

    // site.provide.attr(selector, attr) — real method, real reply shape ({ok,data})
    QJsonObject attrReply = browser.call(name, "provide.attr",
        {"form#wrapper input[name=csrftkn]", "value"});
    QString csrfToken = attrReply.value("data").toString();

    if (csrfToken.isEmpty()) {
        std::cerr << "[movie_source2] no csrf token found for episode " << id << "\n";
        browser.shutdown();
        return "";
    }

    // Uses epId here (previous version had this hardcoded to a stray example slug)
    browser.call(name, "navigate",
        {QString("%1/proxy/player/%2?csrftkn=%3").arg(base, epId, csrfToken)});

    // site.session.export() — real method name
    QJsonObject sessionReply = browser.call(name, "session.export");
    QString sessionCookie = sessionReply.value("data").toString();

    browser.shutdown();

    if (sessionCookie.isEmpty()) {
        std::cerr << "[movie_source2] no session cookie for episode " << id << "\n";
        return "";
    }

    return QString("%1/proxy/nocache/%2/|Cookie=session=%3")
        .arg(base, epId, sessionCookie)
        .toStdString();
}

std::vector<std::string> AnimeCloudProvider::getSubtitleUrls(const std::string& id) {
    // GetEpisode's response for this episode id likely carries a subtitles
    // array — re-fetch it here since ISourceProvider gives no shared cache.
    json epBody = {{"slug", id}};
    std::string raw = httpPost("/api.v1.anime.AnimeService/GetEpisode", epBody.dump());
    if (raw.empty()) return {};

    std::vector<std::string> subs;
    try {
        json epData = json::parse(raw);
        for (auto& sub : epData.value("subtitles", json::array())) {
            subs.push_back(sub.value("url", ""));
        }
    } catch (const json::exception& e) {
        std::cerr << "[movie_source2] getSubtitleUrls parse error: " << e.what() << "\n";
    }
    return subs;
}

}