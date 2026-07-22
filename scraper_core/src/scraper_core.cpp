#include "scraper_core/ScraperEngine.h"
#include <iostream>

namespace scraper_core {

std::string ScraperEngine::vendorBinaryPath() const {
#if defined(_WIN32)
    return "vendor/nothing-browser/nothing-browser.exe";
#else
    return "vendor/nothing-browser/nothing-browser";
#endif
}

void ScraperEngine::loadPage(const std::string& url) {
    std::cout << "[scraper_core] would launch " << vendorBinaryPath()
              << " to load: " << url << std::endl;
    // TODO: spawn as subprocess (QProcess) in headless/automation mode,
    // read captured network requests via nothing-browser's Piggy interface
    // or its exported network capture log, not by linking internals.
}

std::string ScraperEngine::extractStreamLink() {
    // TODO: parse captured requests for m3u8/mp4 manifest URLs
    return "";
}

}
