#include "tmdb_client/TmdbClient.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>

using json = nlohmann::json;

namespace tmdb_client {

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

static std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

TmdbClient::TmdbClient(std::string apiKey) : apiKey_(std::move(apiKey)) {}

// simple normalized Levenshtein-based similarity, 0.0 - 1.0
double TmdbClient::titleSimilarity(const std::string& a, const std::string& b) const {
    std::string x = toLower(a), y = toLower(b);
    size_t m = x.size(), n = y.size();
    if (m == 0 || n == 0) return 0.0;

    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    for (size_t i = 0; i <= m; ++i) dp[i][0] = (int)i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = (int)j;

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            int cost = (x[i - 1] == y[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({ dp[i-1][j] + 1, dp[i][j-1] + 1, dp[i-1][j-1] + cost });
        }
    }

    int distance = dp[m][n];
    int maxLen = (int)std::max(m, n);
    return 1.0 - (double)distance / maxLen;
}

std::string TmdbClient::fetchImdbId(const std::string& tmdbId, bool isMovie) const {
    std::string type = isMovie ? "movie" : "tv";
    std::string url = "https://api.themoviedb.org/3/" + type + "/" + tmdbId +
                       "/external_ids?api_key=" + apiKey_;
    std::string body = httpGet(url);

    try {
        json j = json::parse(body);
        return j.value("imdb_id", "");
    } catch (...) {
        return "";
    }
}

TmdbMatch TmdbClient::searchBestMatch(const std::string& cleanedTitle, int year) {
    TmdbMatch best;

    // try multi-search first: covers both movie and tv in one call
    std::string url = "https://api.themoviedb.org/3/search/multi?api_key=" + apiKey_ +
                       "&query=" + urlEncode(cleanedTitle);
    std::string body = httpGet(url);

    json results;
    try {
        json j = json::parse(body);
        results = j["results"];
    } catch (...) {
        return best; // found = false
    }

    double bestScore = 0.0;

    for (auto& r : results) {
        std::string mediaType = r.value("media_type", "");
        if (mediaType != "movie" && mediaType != "tv") continue; // skip "person" results

        bool isMovie = (mediaType == "movie");
        std::string title = isMovie ? r.value("title", "") : r.value("name", "");
        std::string dateStr = isMovie ? r.value("release_date", "") : r.value("first_air_date", "");
        int resultYear = 0;
        if (dateStr.size() >= 4) {
            try { resultYear = std::stoi(dateStr.substr(0, 4)); } catch (...) {}
        }

        double score = titleSimilarity(cleanedTitle, title);

        // boost score if year matches (or is close, TV can be a year off from first air date)
        if (year > 0 && resultYear > 0) {
            if (resultYear == year) score += 0.15;
            else if (std::abs(resultYear - year) <= 1) score += 0.05;
        }

        if (score > bestScore) {
            bestScore = score;
            best.officialTitle = title;
            best.year = resultYear;
            best.isMovie = isMovie;
            std::string posterPath = r.value("poster_path", "");
            best.posterUrl = posterPath.empty() ? "" : "https://image.tmdb.org/t/p/w500" + posterPath;

            std::string tmdbId = std::to_string(r.value("id", 0));
            best.imdbId = fetchImdbId(tmdbId, isMovie);
        }
    }

    best.confidence = bestScore;
    best.found = bestScore >= kConfidenceThreshold;
    return best;
}

std::vector<TmdbListItem> TmdbClient::fetchList(const std::string& endpoint) const {
    std::vector<TmdbListItem> items;

    std::string url = "https://api.themoviedb.org/3/" + endpoint + "?api_key=" + apiKey_;
    std::string body = httpGet(url);

    try {
        json j = json::parse(body);
        for (auto& r : j["results"]) {
            TmdbListItem item;
            item.tmdbId = r.value("id", 0);
            item.title = r.value("title", "");
            std::string posterPath = r.value("poster_path", "");
            item.posterUrl = posterPath.empty() ? "" : "https://image.tmdb.org/t/p/w500" + posterPath;

            std::string backdropPath = r.value("backdrop_path", "");
            item.backdropUrl = backdropPath.empty() ? "" : "https://image.tmdb.org/t/p/w1280" + backdropPath;

            std::string dateStr = r.value("release_date", "");
            if (dateStr.size() >= 4) {
                try { item.year = std::stoi(dateStr.substr(0, 4)); } catch (...) {}
            }
            items.push_back(std::move(item));
        }
    } catch (...) {
        // return whatever was parsed before failure, or empty
    }

    return items;
}

std::vector<TmdbListItem> TmdbClient::getTrending() {
    return fetchList("trending/movie/week");
}

std::vector<TmdbListItem> TmdbClient::getUpcoming() {
    return fetchList("movie/upcoming");
}

}