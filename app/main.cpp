#include <QApplication>
#include <QCoreApplication>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ui/MainWindow.h"
#include "ui/TmdbBridge.h"
#include "ui/SearchBridge.h"
#include "ui/HomepageBridge.h"
#include "ui/AppController.h"
#include "ui/QueueBridge.h"
#include "search_aggregator/search_aggregator.h"
#include "queue_manager/queue_manager.h"
#include "movie_source1/movie_source1.h"
#include "movie_source3/movie_source3.h"
#include "movie_source2/movie_source2.h"
#include "scraper_core/ScraperEngine.h"   // for NothingBrowser and isAvailable
// NOTE: vendor_updater/VendorUpdater.h is intentionally NOT included here
// anymore. scraper_core owns the entire vendor install/update lifecycle
// for the Nothing Browser binary internally now (see NothingBrowser::start()) -
// this file used to construct its own separate VendorUpdater pointed at a
// different directory ("vendor/nothing-browser", relative to cwd) than the
// one scraper_core actually checks ("<appDir>/nothing"), which silently
// diverged and caused the binary to never be found. One owner, one path.

namespace {

std::map<std::string, std::string> parseEnvFile(const std::filesystem::path& path) {
    std::map<std::string, std::string> values;
    std::ifstream file(path);
    if (!file.is_open()) {
        return values;
    }

    auto trim = [](std::string& s) {
        auto first = s.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            s.clear();
            return;
        }
        auto last = s.find_last_not_of(" \t\r\n");
        s = s.substr(first, last - first + 1);
    };

    std::string line;
    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        trim(key);
        trim(value);
        if (value.size() >= 2 &&
            (value.front() == '"' || value.front() == '\'') &&
            value.back() == value.front()) {
            value = value.substr(1, value.size() - 2);
        }
        if (!key.empty()) {
            values[key] = value;
        }
    }
    return values;
}

std::string resolveTmdbApiKey() {
    std::vector<std::filesystem::path> candidates;

    const auto exeDir =
        std::filesystem::path(QCoreApplication::applicationDirPath().toStdString());
    candidates.push_back(exeDir / ".env");

    auto dir = exeDir;
    for (int i = 0; i < 6; ++i) {
        if (!dir.has_parent_path() || dir == dir.parent_path()) {
            break;
        }
        dir = dir.parent_path();
        candidates.push_back(dir / ".env");
    }

    for (const auto& path : candidates) {
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            continue;
        }
        auto values = parseEnvFile(path);
        auto it = values.find("TMDB_API_KEY");
        if (it != values.end() && !it->second.empty()) {
            std::cout << "[env] TMDB_API_KEY loaded from " << path.string() << "\n";
            return it->second;
        }
    }

    if (const char* fromEnv = std::getenv("TMDB_API_KEY")) {
        std::cout << "[env] TMDB_API_KEY loaded from process environment\n";
        return std::string(fromEnv);
    }

    return {};
}

// ---------------------------------------------------------------------
// Movie source registration.
// ---------------------------------------------------------------------
std::shared_ptr<search_aggregator::SearchAggregatorModule> buildSourceAggregator(scraper_core::NothingBrowser* browser) {
    auto aggregator = std::make_shared<search_aggregator::SearchAggregatorModule>();

    aggregator->registerSource("Apibay (Torrent)", std::make_shared<movie_source1::ApibayProvider>());
    aggregator->registerSource("MockSource3", std::make_shared<movie_source3::MockSourceProvider>());
    aggregator->registerSource("Aniworld", std::make_shared<movie_source2::AniworldProvider>(browser));

    return aggregator;
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::cout << "Nothing Movies - We have what? Everything.\n";

    const std::string tmdbApiKey = resolveTmdbApiKey();
    if (tmdbApiKey.empty()) {
        std::cerr << "[env] warning: TMDB_API_KEY not found (.env or process env) — "
                      "Trending/Upcoming carousels and search matching will fail to load\n";
    }

    // Create and start the Nothing Browser. This now also handles fetching
    // and keeping the vendored binary up to date internally - no separate
    // VendorUpdater needed here.
    auto browser = std::make_shared<scraper_core::NothingBrowser>();
    if (!browser->start()) {
        std::cerr << "[scraper_core] Failed to start Nothing Browser!\n";
    } else {
        std::cout << "[scraper_core] Nothing Browser started successfully.\n";
    }

    auto aggregator = buildSourceAggregator(browser.get());

    ui::TmdbBridge tmdbBridge(tmdbApiKey);
    ui::SearchBridge searchBridge(tmdbApiKey, aggregator);
    ui::HomepageBridge homepageBridge(aggregator, &tmdbBridge);

    auto queueManager = std::make_shared<queue_manager::Queue_managerModule>();
    queueManager->init();

    ui::AppController appController;
    ui::QueueBridge queueBridge(queueManager);

    ui::MainWindow window(&tmdbBridge, &searchBridge, &appController, &queueBridge, queueManager, &homepageBridge);
    window.show();
    return app.exec();
}