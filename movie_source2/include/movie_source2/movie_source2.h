#pragma once
#include "core/ISourceProvider.h"
#include <string>
#include <vector>

namespace movie_source2 {

class KflixProvider : public core::ISourceProvider {
public:
    KflixProvider() = default;

    core::SourceCapabilities        getCapabilities() const override;
    std::vector<core::MediaResult>  search(const std::string& query) override;
    std::vector<core::MediaResult>  searchTV(const std::string& query) override;
    std::string                     getStreamUrl(const std::string& id) override;
    std::string                     getTVStreamUrl(const std::string& id,
                                                   int season, int episode) override;
    std::vector<core::HomepageItem> getHomepage() override;
    core::MediaInfo                 getMediaInfo(const std::string& id) override;
    core::MediaInfo                 getTVInfo(const std::string& id) override;
    std::vector<core::DownloadOption> getDownloadOptions(const std::string& title, int year) override;
    std::vector<core::DownloadOption> getTVDownloadOptions(const std::string& title, int year) override;
    std::vector<std::string>        getSubtitleUrls(const std::string& id) override;

private:
    std::string httpGet(const std::string& url) const;
    std::string urlEncode(const std::string& s) const;

    static constexpr const char* KFLIX_BASE   = "https://api.kflix.cc/api/v1";
    static constexpr const char* VIDLOVE_BASE = "https://player.vidlove.cc/embed/movie";
};

} // namespace movie_source2