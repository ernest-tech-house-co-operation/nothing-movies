#pragma once
#include "core/ISourceProvider.h"
#include <string>
#include <vector>

namespace movie_source2 {

class AnimeCloudProvider : public core::ISourceProvider {
public:
    AnimeCloudProvider() = default;

    // Not part of ISourceProvider (no init() there) — call this once
    // after construction, same as movie_source1/3 presumably do in main.cpp.
    bool init();

    core::SourceCapabilities        getCapabilities() const override;
    std::vector<core::HomepageItem> getHomepage() override;
    std::vector<core::MediaResult>  search(const std::string& query) override;
    core::MediaInfo                 getMediaInfo(const std::string& id) override;
    std::string                     getStreamUrl(const std::string& id) override;
    std::vector<std::string>        getSubtitleUrls(const std::string& id) override;

private:
    std::string m_baseUrl = "https://fireani.me";
    std::string m_siteName = "animecloud"; // name used to register/address this site with Piggy
    mutable core::SourceCapabilities m_capabilities;
    mutable bool m_capabilitiesLoaded = false;

    std::string httpPost(const std::string& endpoint, const std::string& jsonBody) const;
    std::string dataFilePath(const std::string& filename) const;
};

}