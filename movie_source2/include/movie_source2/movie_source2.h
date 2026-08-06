#pragma once

#include "core/ISourceProvider.h"
#include "scraper_core/ScraperEngine.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace movie_source2 {

using json = nlohmann::json;

class AniworldProvider : public core::ISourceProvider {
public:
    explicit AniworldProvider(scraper_core::NothingBrowser* engine);
    ~AniworldProvider() override = default;

    core::SourceCapabilities getCapabilities() const override;
    std::vector<core::HomepageItem> getHomepage() override;
    core::MediaInfo getMediaInfo(const std::string& id) override;
    std::string getStreamUrl(const std::string& id) override;
    std::vector<core::MediaResult> search(const std::string& query) override;
    std::vector<std::string> getSubtitleUrls(const std::string& id) override;

    void setToken(const std::string& token);

private:
    scraper_core::NothingBrowser* m_engine;
    std::string m_token;
    bool m_isSerienstream = false;

    // Browser helpers
    std::string sendBrowserCommand(const std::string& cmd, const json& payload = {}, int timeoutMs = 15000);
    json sendBrowserCommandJson(const std::string& cmd, const json& payload = {}, int timeoutMs = 15000);
    std::string createTab();
    void closeTab(const std::string& tabId);
    std::string navigateAndWait(const std::string& tabId, const std::string& url);
    std::string getPageContent(const std::string& tabId);
    std::string executeScript(const std::string& tabId, const std::string& script);

    // Constants
    static constexpr const char* MAIN_URL = "https://aniworld.to";
    static constexpr const char* SERIENSTREAM_URL = "https://serienstream.to";
};

} // namespace movie_source2