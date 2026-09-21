#pragma once
#include "core/ISourceProvider.h"
#include <string>
#include <vector>

namespace movie_source3 {

class RivestreamProvider : public core::ISourceProvider {
public:
    RivestreamProvider() = default;

    core::SourceCapabilities        getCapabilities() const override;
    std::vector<core::MediaResult>  search(const std::string& query) override;
    std::string                     getStreamUrl(const std::string& id) override;
    std::vector<core::HomepageItem> getHomepage() override { return {}; }
    core::MediaInfo                 getMediaInfo(const std::string&) override { return {}; }
    std::vector<std::string>        getSubtitleUrls(const std::string& id) override;

private:
    std::string httpGet(const std::string& url) const;
    std::string generateSecretKey(const std::string& id) const;

    static constexpr const char* RIVESTREAM_BASE = "https://backend.rivestream.net/api";
    static constexpr const char* SERVICES[] = {
        "apex", "quasar", "vanguard", "aura", "solstice", "primevids", "citadel", "asiacloud"
    };
};

} // namespace movie_source3