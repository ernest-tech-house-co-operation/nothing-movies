#pragma once
#include <string>
#include <vector>

namespace core {

struct MediaResult {
    std::string id;
    std::string title;
    std::string posterUrl;
    std::string sourceName;
    std::string backdropUrl;
    std::string genre;
    double rating = 0.0;
    int year = 0;
};

struct DownloadOption {
    std::string magnetUrl;
    std::string title;
    std::string quality;
    std::string size;
    int seeds = 0;
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
    bool hasSoundtrack = false;
    bool hasSimilar   = false;
};

struct SourceCapabilities {
    bool hasHomepage   = false;
    bool hasPoster     = false;
    bool hasRating     = false;
    bool hasSubtitles  = false;
    bool hasDownload   = false;
    bool hasStream     = false;
    bool hasInfo       = false;
    bool hasTV         = false;   // source supports TV/series search
    bool hasTrending   = false;   // source has a trending/homepage feed
    std::string streamType    = "";
    std::string downloadType  = "";
    std::string version       = "";
    std::string profilePicture = "";
    InfoCapabilities info;
};

struct HomepageItem {
    std::string id;
    std::string title;
    std::string imageUrl;
    std::string backdropUrl;
    std::string category;
    std::string trailerLink;
    std::string genre;
    double rating = 0.0;
    int year = 0;
};

struct CastMember {
    std::string name;
    std::string character;
    std::string profileUrl;
};

struct SoundtrackTrack {
    std::string id;
    std::string title;
    std::string artist;
    std::string thumbnailUrl;
    int durationSecs = 0;
};

struct MediaInfo {
    std::string id;
    std::string title;
    std::string type;        // "movie" or "series"
    std::string posterUrl;
    std::string backdropUrl;
    std::string synopsis;
    std::string trailerUrl;
    std::string trailerKey;  // YouTube key for embedded player
    std::string quality;
    std::string imdbId;
    int year       = 0;
    int lengthMins = 0;
    double rating  = 0.0;

    std::vector<CastMember>      cast;
    std::vector<std::string>     subtitleUrls;
    std::vector<MediaInfo>       similar;      // lightweight — only id/title/posterUrl filled
    std::vector<SoundtrackTrack> soundtrack;

    struct Episode {
        int season  = 0;
        int episode = 0;
        std::string title;
        std::string synopsis;
        std::string streamUrl;
        std::string stillUrl;
    };
    std::vector<Episode> episodes;
};

class ISourceProvider {
public:
    virtual ~ISourceProvider() = default;
    virtual std::vector<MediaResult>  search(const std::string& query) = 0;
    virtual std::vector<MediaResult>  searchTV(const std::string& query) { return {}; }
    virtual std::string               getStreamUrl(const std::string& id) = 0;
    virtual std::string               getTVStreamUrl(const std::string& id,
                                                     int season, int episode) { return ""; }
    virtual SourceCapabilities        getCapabilities() const = 0;
    virtual std::vector<HomepageItem> getHomepage() = 0;
    virtual MediaInfo                 getMediaInfo(const std::string& id) = 0;
    virtual MediaInfo                 getTVInfo(const std::string& id) { return getMediaInfo(id); }
    virtual std::vector<DownloadOption> getDownloadOptions(const std::string&, int) { return {}; }
    virtual std::vector<DownloadOption> getTVDownloadOptions(const std::string&, int) { return {}; }
    virtual std::vector<std::string>  getSubtitleUrls(const std::string& id) = 0;
};

} // namespace core