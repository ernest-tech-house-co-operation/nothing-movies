#pragma once
#include <string>
#include <vector>
#include <optional>

namespace tmdb_client {

struct TmdbMatch {
    bool found = false;
    std::string officialTitle;
    std::string posterUrl;
    std::string imdbId;
    int year = 0;
    double confidence = 0.0; // 0.0 - 1.0, based on title similarity + year match
    bool isMovie = true;     // false if it's a TV series
};

struct TmdbListItem {
    int tmdbId = 0;
    std::string title;
    std::string posterUrl;
    std::string backdropUrl;
    int year = 0;
};

class TmdbClient {
public:
    explicit TmdbClient(std::string apiKey);

    // searches TMDb for cleanedTitle, optionally narrowed by year,
    // and returns the best match if confidence clears the threshold
    TmdbMatch searchBestMatch(const std::string& cleanedTitle, int year = 0);

    // for Home screen carousels -- direct TMDB list endpoints, no fuzzy matching needed
    std::vector<TmdbListItem> getTrending();
    std::vector<TmdbListItem> getUpcoming();

private:
    std::string apiKey_;
    static constexpr double kConfidenceThreshold = 0.6;

    double titleSimilarity(const std::string& a, const std::string& b) const;
    std::string fetchImdbId(const std::string& tmdbId, bool isMovie) const;
    std::vector<TmdbListItem> fetchList(const std::string& endpoint) const;
};

}