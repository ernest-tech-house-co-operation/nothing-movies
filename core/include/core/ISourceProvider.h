#pragma once
#include <string>
#include <vector>

namespace core {

struct MediaResult {
    std::string id;
    std::string title;
    std::string posterUrl;
    std::string sourceName;
};

struct InfoCapabilities {
    bool hasMovie     = false;
    bool hasSeries    = false;
    bool hasYear      = false;
    bool hasRating    = false;
    bool hasCast      = false;
    bool hasSynopsis  = false;
    bool hasTrailer   = false;
    bool hasQuality   = false;
    bool hasLength    = false;
    bool hasSubtitles = false;
};

struct SourceCapabilities {
    bool hasHomepage  = false;
    bool hasPoster    = false;
    bool hasRating    = false;
    bool hasSubtitles = false;
    bool hasDownload  = false;
    bool hasStream    = false;
    bool hasInfo      = false;
    std::string streamType   = "";  // "http" or "torrent"
    std::string downloadType = "";
    std::string version        = "";
    std::string profilePicture = "";
    InfoCapabilities info;
};

struct HomepageItem {
    std::string id;
    std::string title;
    std::string imageUrl;
    std::string category;
    std::string trailerLink;
    std::string genre;
    double rating = 0.0;
};

struct MediaInfo {
    std::string id;
    std::string title;
    std::string type;        // "movie" or "series" — per title, set by source
    std::string posterUrl;
    std::string backdropUrl;
    std::string synopsis;
    std::string trailerUrl;
    std::string quality;
    int year       = 0;
    int lengthMins = 0;
    double rating  = 0.0;
    std::vector<std::string> cast;
    std::vector<std::string> subtitleUrls;

    struct Episode {
        int season  = 0;
        int episode = 0;
        std::string title;
        std::string synopsis;
        std::string streamUrl;
    };
    std::vector<Episode> episodes; // empty for movies
};

class ISourceProvider {
public:
    virtual ~ISourceProvider() = default;
    virtual std::vector<MediaResult>  search(const std::string& query) = 0;
    virtual std::string               getStreamUrl(const std::string& id) = 0;
    virtual SourceCapabilities        getCapabilities() const = 0;
    virtual std::vector<HomepageItem> getHomepage() = 0;
    virtual MediaInfo                 getMediaInfo(const std::string& id) = 0;
    virtual std::vector<std::string>  getSubtitleUrls(const std::string& id) = 0;
};

} // namespace core