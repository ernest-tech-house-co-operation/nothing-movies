#include "movie_source3/movie_source3.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace movie_source3 {

using json = nlohmann::json;

MockSourceProvider::MockSourceProvider() {
    std::ifstream infoFile("movie_source3/data/info.json");
    if (infoFile) {
        json j; infoFile >> j;
        capabilities_.hasHomepage  = j.value("hasHomepage", false);   // NEW
        capabilities_.hasPoster    = j.value("hasPoster", false);
        capabilities_.hasRating    = j.value("hasRating", false);
        capabilities_.hasSubtitles = j.value("hasSubtitles", false);
        capabilities_.hasDownload  = j.value("hasDownload", false);
        capabilities_.hasStream    = j.value("hasStream", false);
        capabilities_.streamType   = j.value("streamType", "");
        capabilities_.downloadType = j.value("downloadType", "");
    }

    std::ifstream homeFile("movie_source3/data/mock_homepage.json");
    if (homeFile) {
        json arr; homeFile >> arr;
        for (const auto& item : arr) {
            core::HomepageItem h;
            h.id = item.value("id", "");
            h.title = item.value("title", "");
            h.imageUrl = item.value("imageUrl", "");
            h.category = item.value("category", "");
            h.trailerLink = item.value("trailerLink", "");
            h.genre = item.value("genre", "");
            h.rating = item.value("rating", 0.0);
            homepageItems_.push_back(h);
        }
    }
}

std::vector<core::MediaResult> MockSourceProvider::search(const std::string&) {
    return {};  // not implemented for this test source
}

std::string MockSourceProvider::getStreamUrl(const std::string&) {
    return "";  // not implemented for this test source
}

core::SourceCapabilities MockSourceProvider::getCapabilities() const {
    return capabilities_;
}

std::vector<core::HomepageItem> MockSourceProvider::getHomepage() {
    return homepageItems_;
}

}