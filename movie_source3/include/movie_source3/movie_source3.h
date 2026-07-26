#pragma once
#include "core/ISourceProvider.h"

namespace movie_source3 {

class MockSourceProvider : public core::ISourceProvider {
public:
    MockSourceProvider();

    std::vector<core::MediaResult> search(const std::string& query) override;
    std::string getStreamUrl(const std::string& id) override;
    core::SourceCapabilities getCapabilities() const override;
    std::vector<core::HomepageItem> getHomepage() override;
    core::MediaInfo getMediaInfo(const std::string& id) override;
    std::vector<std::string> getSubtitleUrls(const std::string& id) override;

private:
    core::SourceCapabilities capabilities_;
    std::vector<core::HomepageItem> homepageItems_;
};

}