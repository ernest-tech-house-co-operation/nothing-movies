#include "movie_source2/movie_source2.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace movie_source2 {

static size_t writeCallback(void* c, size_t s, size_t n, void* out) {
    static_cast<std::string*>(out)->append((char*)c, s * n);
    return s * n;
}

std::string KflixProvider::httpGet(const std::string& url) const {
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

std::string KflixProvider::urlEncode(const std::string& s) const {
    CURL* curl = curl_easy_init();
    std::string result;
    if (curl) {
        char* enc = curl_easy_escape(curl, s.c_str(), (int)s.size());
        if (enc) { result = enc; curl_free(enc); }
        curl_easy_cleanup(curl);
    }
    return result;
}

core::SourceCapabilities KflixProvider::getCapabilities() const {
    core::SourceCapabilities caps;
    caps.hasHomepage    = true;
    caps.hasPoster      = true;
    caps.hasRating      = true;
    caps.hasSubtitles   = false;
    caps.hasDownload    = true;
    caps.hasStream      = true;
    caps.hasInfo        = true;
    caps.hasTV          = true;
    caps.hasTrending    = true;
    caps.streamType     = "embed";
    caps.downloadType   = "torrent";
    caps.version        = "1.0.0";
    caps.info.hasMovie      = true;
    caps.info.hasSeries     = true;
    caps.info.hasYear       = true;
    caps.info.hasRating     = true;
    caps.info.hasCast       = true;
    caps.info.hasSynopsis   = true;
    caps.info.hasTrailer    = true;
    caps.info.hasLength     = true;
    caps.info.hasSoundtrack = true;
    caps.info.hasSimilar    = true;
    return caps;
}

std::vector<core::HomepageItem> KflixProvider::getHomepage() {
    std::vector<core::HomepageItem> results;
    std::string body = httpGet(std::string(KFLIX_BASE) + "/movies/trending");
    std::cerr << "[Kflix] getHomepage() called, body size: " << body.size() << std::endl;
    try {
        json data = json::parse(body);
        if (!data.value("success", false)) return results;
        auto& content = data["content"];
        if (!content.is_array()) return results;
        for (const auto& item : content) {
            core::HomepageItem h;
            h.id     = std::to_string(item.value("id", 0));
            h.title  = item.value("title", "Unknown");
            h.rating = item.value("vote_average", 0.0);
            std::string rd = item.value("release_date", "");
            if (rd.size() >= 4) { try { h.year = std::stoi(rd.substr(0,4)); } catch(...){} }
            if (item.contains("poster_path") && !item["poster_path"].is_null())
                h.imageUrl = "https://image.tmdb.org/t/p/w500" + item["poster_path"].get<std::string>();
            if (item.contains("backdrop_path") && !item["backdrop_path"].is_null())
                h.backdropUrl = "https://image.tmdb.org/t/p/original" + item["backdrop_path"].get<std::string>();
            results.push_back(h);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Kflix] homepage parse error: " << e.what() << std::endl;
    }
    return results;
}

std::vector<core::MediaResult> KflixProvider::search(const std::string& query) {
    std::vector<core::MediaResult> results;
    std::string body = httpGet(std::string(KFLIX_BASE) + "/search/movie/" + urlEncode(query));
    try {
        json data = json::parse(body);
        if (!data.value("success", false)) return results;
        if (!data.contains("content") || !data["content"].is_array()) return results;
        for (const auto& item : data["content"]) {
            core::MediaResult r;
            r.id         = std::to_string(item.value("id", 0));
            r.title      = item.value("title", "Unknown");
            r.sourceName = "Kflix";
            r.rating     = item.value("vote_average", 0.0);
            std::string rd = item.value("release_date", "");
            if (rd.size() >= 4) { try { r.year = std::stoi(rd.substr(0,4)); } catch(...){} }
            if (item.contains("poster_path") && !item["poster_path"].is_null())
                r.posterUrl = "https://image.tmdb.org/t/p/w500" + item["poster_path"].get<std::string>();
            if (item.contains("backdrop_path") && !item["backdrop_path"].is_null())
                r.backdropUrl = "https://image.tmdb.org/t/p/original" + item["backdrop_path"].get<std::string>();
            results.push_back(r);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Kflix] search parse error: " << e.what() << std::endl;
    }
    return results;
}

std::vector<core::MediaResult> KflixProvider::searchTV(const std::string& query) {
    std::vector<core::MediaResult> results;
    std::string body = httpGet(std::string(KFLIX_BASE) + "/search/tv/" + urlEncode(query));
    try {
        json data = json::parse(body);
        if (!data.value("success", false)) return results;
        if (!data.contains("content") || !data["content"].is_array()) return results;
        for (const auto& item : data["content"]) {
            core::MediaResult r;
            r.id         = std::to_string(item.value("id", 0));
            r.title      = item.value("name", item.value("title", "Unknown"));
            r.sourceName = "Kflix";
            r.rating     = item.value("vote_average", 0.0);
            std::string rd = item.value("first_air_date", "");
            if (rd.size() >= 4) { try { r.year = std::stoi(rd.substr(0,4)); } catch(...){} }
            if (item.contains("poster_path") && !item["poster_path"].is_null())
                r.posterUrl = "https://image.tmdb.org/t/p/w500" + item["poster_path"].get<std::string>();
            results.push_back(r);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Kflix] searchTV parse error: " << e.what() << std::endl;
    }
    return results;
}

std::vector<core::DownloadOption> KflixProvider::getDownloadOptions(
    const std::string& title, int year) {
    std::vector<core::DownloadOption> results;
    const std::string url = "https://en.yts-official.com/?api=torrents&mode=movie&name=" +
                            urlEncode(title) + "&year=" + std::to_string(year) + "&quality=all";
    const std::string body = httpGet(url);
    try {
        json data = json::parse(body);
        if (!data.contains("hits") || !data["hits"].is_array()) return results;
        for (const auto& hit : data["hits"]) {
            core::DownloadOption option;
            option.magnetUrl = hit.value("magnetUrl", "");
            if (option.magnetUrl.empty()) continue;
            option.title   = hit.value("title", "");
            option.quality = hit.value("quality", "");
            option.seeds   = hit.value("seeds", 0);
            option.size    = std::to_string(hit.value("bytes", 0LL));
            results.push_back(option);
        }
    } catch (...) {}
    return results;
}

std::vector<core::DownloadOption> KflixProvider::getTVDownloadOptions(
    const std::string& title, int year) {
    std::vector<core::DownloadOption> results;
    const std::string url = "https://en.yts-official.com/?api=torrents&mode=tv&name=" +
                            urlEncode(title) + "&year=" + std::to_string(year) + "&quality=all";
    const std::string body = httpGet(url);
    try {
        json data = json::parse(body);
        if (!data.contains("hits") || !data["hits"].is_array()) return results;
        for (const auto& hit : data["hits"]) {
            core::DownloadOption option;
            option.magnetUrl = hit.value("magnetUrl", "");
            if (option.magnetUrl.empty()) continue;
            option.title   = hit.value("title", "");
            option.quality = hit.value("quality", "");
            option.seeds   = hit.value("seeds", 0);
            option.size    = std::to_string(hit.value("bytes", 0LL));
            results.push_back(option);
        }
    } catch (...) {}
    return results;
}

core::MediaInfo KflixProvider::getMediaInfo(const std::string& id) {
    core::MediaInfo info;
    std::string body = httpGet(std::string(KFLIX_BASE) + "/movies/details/" + id);
    try {
        json data = json::parse(body);
        if (!data.value("success", false)) return info;
        auto& c = data["content"];
        info.id         = id;
        info.title      = c.value("title", "Unknown");
        info.synopsis   = c.value("overview", "");
        info.lengthMins = c.value("runtime", 0);
        info.rating     = c.value("vote_average", 0.0);
        info.imdbId     = c.value("imdb_id", "");
        info.type       = "movie";
        std::string rd = c.value("release_date", "");
        if (rd.size() >= 4) { try { info.year = std::stoi(rd.substr(0,4)); } catch(...){} }
        if (c.contains("poster_path") && !c["poster_path"].is_null())
            info.posterUrl = "https://image.tmdb.org/t/p/w500" + c["poster_path"].get<std::string>();
        if (c.contains("backdrop_path") && !c["backdrop_path"].is_null())
            info.backdropUrl = "https://image.tmdb.org/t/p/original" + c["backdrop_path"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "[Kflix] details parse error: " << e.what() << std::endl;
        return info;
    }

    try {
        std::string tbody = httpGet(std::string(KFLIX_BASE) + "/movies/trailers/" + id);
        json tdata = json::parse(tbody);
        if (tdata.value("success", false) && tdata.contains("content") && tdata["content"].is_array()) {
            for (const auto& v : tdata["content"]) {
                if (v.value("site", "") == "YouTube") {
                    info.trailerKey = v.value("key", "");
                    info.trailerUrl = "https://www.youtube.com/watch?v=" + info.trailerKey;
                    break;
                }
            }
        }
    } catch (...) {}

    try {
        std::string cbody = httpGet(std::string(KFLIX_BASE) + "/movies/credits/" + id);
        json cdata = json::parse(cbody);
        if (cdata.value("success", false) && cdata.contains("content")) {
            auto& castArr = cdata["content"].contains("cast") ? cdata["content"]["cast"] : cdata["content"];
            if (castArr.is_array()) {
                for (const auto& actor : castArr) {
                    core::CastMember m;
                    m.name      = actor.value("name", "");
                    m.character = actor.value("character", "");
                    if (actor.contains("profile_path") && !actor["profile_path"].is_null())
                        m.profileUrl = "https://image.tmdb.org/t/p/w185" + actor["profile_path"].get<std::string>();
                    info.cast.push_back(m);
                }
            }
        }
    } catch (...) {}

    try {
        std::string sbody = httpGet(std::string(KFLIX_BASE) + "/movies/similar/" + id);
        json sdata = json::parse(sbody);
        if (sdata.value("success", false) && sdata.contains("content") && sdata["content"].is_array()) {
            for (const auto& item : sdata["content"]) {
                core::MediaInfo sim;
                sim.id    = std::to_string(item.value("id", 0));
                sim.title = item.value("title", "Unknown");
                sim.type  = "movie";
                if (item.contains("poster_path") && !item["poster_path"].is_null())
                    sim.posterUrl = "https://image.tmdb.org/t/p/w500" + item["poster_path"].get<std::string>();
                info.similar.push_back(sim);
            }
        }
    } catch (...) {}

    try {
        std::string stbody = httpGet(std::string(KFLIX_BASE) + "/movies/soundtrack/" + id);
        json stdata = json::parse(stbody);
        if (stdata.contains("content") && stdata["content"].contains("tracks")) {
            for (const auto& t : stdata["content"]["tracks"]) {
                core::SoundtrackTrack track;
                track.id           = t.value("id", "");
                track.title        = t.value("title", "");
                track.artist       = t.value("artist", "");
                track.thumbnailUrl = t.value("thumbnail", "");
                track.durationSecs = t.value("duration", 0);
                info.soundtrack.push_back(track);
            }
        }
    } catch (...) {}

    return info;
}

core::MediaInfo KflixProvider::getTVInfo(const std::string& id) {
    core::MediaInfo info;
    std::string body = httpGet(std::string(KFLIX_BASE) + "/tv/details/" + id);
    try {
        json data = json::parse(body);
        if (!data.value("success", false)) return info;
        auto& c = data["content"];
        info.id       = id;
        info.title    = c.value("name", c.value("title", "Unknown"));
        info.synopsis = c.value("overview", "");
        info.rating   = c.value("vote_average", 0.0);
        info.type     = "tv";
        std::string rd = c.value("first_air_date", "");
        if (rd.size() >= 4) { try { info.year = std::stoi(rd.substr(0,4)); } catch(...){} }
        if (c.contains("poster_path") && !c["poster_path"].is_null())
            info.posterUrl = "https://image.tmdb.org/t/p/w500" + c["poster_path"].get<std::string>();
        if (c.contains("backdrop_path") && !c["backdrop_path"].is_null())
            info.backdropUrl = "https://image.tmdb.org/t/p/original" + c["backdrop_path"].get<std::string>();

        // Parse seasons without fetching individual episode details.
        if (c.contains("seasons") && c["seasons"].is_array()) {
            for (const auto& s : c["seasons"]) {
                int seasonNum = s.value("season_number", 0);
                if (seasonNum == 0) continue;
                int episodeCount = s.value("episode_count", 0);
                for (int ep = 1; ep <= episodeCount; ++ep) {
                    core::MediaInfo::Episode episode;
                    episode.season  = seasonNum;
                    episode.episode = ep;
                    episode.title   = "Episode " + std::to_string(ep);
                    info.episodes.push_back(episode);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Kflix] TV details parse error: " << e.what() << std::endl;
    }

    return info;
}

std::string KflixProvider::getStreamUrl(const std::string& id) {
    return "https://player.vidlove.cc/embed/movie/" + id;
}

std::string KflixProvider::getTVStreamUrl(const std::string& id, int season, int episode) {
    return "https://player.vidlove.cc/embed/tv/" + id + "/" +
           std::to_string(season) + "/" + std::to_string(episode);
}

std::vector<std::string> KflixProvider::getSubtitleUrls(const std::string&) {
    return {};
}

} // namespace movie_source2
// i have the same opinion as you yes i have repeated functions and methods in my code why i dont realy know and more so care it works right YES  so moi am just ok with it i might refactor it later but for now i am just happy it works and i can watch movies and tv shows 
