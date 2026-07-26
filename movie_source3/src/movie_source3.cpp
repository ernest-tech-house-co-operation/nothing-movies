#include "movie_source3/movie_source3.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace movie_source3 {

using json = nlohmann::json;

MockSourceProvider::MockSourceProvider() {
    std::ifstream infoFile("movie_source3/data/info.json");
    if (infoFile) {
        json j; infoFile >> j;
        capabilities_.hasHomepage  = j.value("hasHomepage", false);
        capabilities_.hasPoster    = j.value("hasPoster", false);
        capabilities_.hasRating    = j.value("hasRating", false);
        capabilities_.hasSubtitles = j.value("hasSubtitles", false);
        capabilities_.hasDownload  = j.value("hasDownload", false);
        capabilities_.hasStream    = j.value("hasStream", false);
        capabilities_.hasInfo      = j.value("hasInfo", false);
        capabilities_.streamType   = j.value("streamType", "");
        capabilities_.downloadType = j.value("downloadType", "");
        capabilities_.version        = j.value("version", "");
        capabilities_.profilePicture = j.value("profilePicture", "");

        if (j.contains("info")) {
            const auto& info = j["info"];
            capabilities_.info.hasMovie     = info.value("hasMovie", false);
            capabilities_.info.hasSeries    = info.value("hasSeries", false);
            capabilities_.info.hasYear      = info.value("hasYear", false);
            capabilities_.info.hasRating    = info.value("hasRating", false);
            capabilities_.info.hasCast      = info.value("hasCast", false);
            capabilities_.info.hasSynopsis  = info.value("hasSynopsis", false);
            capabilities_.info.hasTrailer   = info.value("hasTrailer", false);
            capabilities_.info.hasQuality   = info.value("hasQuality", false);
            capabilities_.info.hasLength    = info.value("hasLength", false);
            capabilities_.info.hasSubtitles = info.value("hasSubtitles", false);
        }
    }

    std::ifstream homeFile("movie_source3/data/mock_homepage.json");
    if (homeFile) {
        json arr; homeFile >> arr;
        for (const auto& item : arr) {
            core::HomepageItem h;
            h.id          = item.value("id", "");
            h.title       = item.value("title", "");
            h.imageUrl    = item.value("imageUrl", "");
            h.category    = item.value("category", "");
            h.trailerLink = item.value("trailerLink", "");
            h.genre       = item.value("genre", "");
            h.rating      = item.value("rating", 0.0);
            homepageItems_.push_back(h);
        }
    }
}

std::vector<core::MediaResult> MockSourceProvider::search(const std::string&) {
    return {};
}

std::string MockSourceProvider::getStreamUrl(const std::string&) {
    return "";
}

core::SourceCapabilities MockSourceProvider::getCapabilities() const {
    return capabilities_;
}

std::vector<core::HomepageItem> MockSourceProvider::getHomepage() {
    return homepageItems_;
}

core::MediaInfo MockSourceProvider::getMediaInfo(const std::string& id) {
    core::MediaInfo info;
    info.id          = id;
    info.title       = "Mock Movie Title";
    info.type        = "movie";
    info.posterUrl   = "https://picsum.photos/300/450";
    info.backdropUrl = "https://picsum.photos/1280/720";
    info.synopsis    = "A hardcoded synopsis to prove the info pipeline works end to end.";
    info.quality     = "1080p";
    info.year        = 2024;
    info.lengthMins  = 112;
    info.rating      = 8.1;
    info.cast        = { "Actor One", "Actor Two", "Actor Three" };
    return info;
}

std::vector<std::string> MockSourceProvider::getSubtitleUrls(const std::string&) {
    return {}; // stub — no subtitles in mock data yet
}

} // namespace movie_source3