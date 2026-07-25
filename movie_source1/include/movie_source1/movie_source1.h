#pragma once
#include "core/ISourceProvider.h"
#include <vector>
#include <string>

namespace movie_source1 {

class ApibayProvider : public core::ISourceProvider {
public:
    ApibayProvider() = default;

    std::vector<core::MediaResult> search(const std::string& query) override;
    std::string getStreamUrl(const std::string& id) override;

    // Stubbed — not implemented yet
    core::SourceCapabilities getCapabilities() const override { return {}; }
    std::vector<core::HomepageItem> getHomepage() override { return {}; }
    core::MediaInfo getMediaInfo(const std::string&) override { return {}; }

private:
    std::string detectQuality(const std::string& name) const;
    std::string buildMagnet(const std::string& infoHash, const std::string& displayName) const;
    long long parseSize(const std::string& sizeStr) const; // fixed the std:a:string typo too

    static constexpr long long kMaxSizeBytes = 8LL * 1024 * 1024 * 1024;

    const std::vector<std::string> trackers_ = {
        "udp://tracker.opentrackr.org:1337/announce",
        "udp://tracker.torrent.eu.org:451/announce",
        "udp://exodus.desync.com:6969/announce",
        "udp://tracker.coppersurfer.tk:6969/announce",
    };
};

}