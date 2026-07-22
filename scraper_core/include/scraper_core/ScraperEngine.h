#pragma once
#include <string>

namespace scraper_core {

// Drives the vendored nothing-browser binary (launched as a subprocess,
// not linked in-process) to render movie source pages and capture
// network traffic for real stream URLs.
class ScraperEngine {
public:
    ScraperEngine() = default;
    void loadPage(const std::string& url);
    std::string extractStreamLink();

private:
    std::string vendorBinaryPath() const;
};

}
