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
#include "ui/AppController.h"
#include "ui/QueueBridge.h"
#include "vendor_updater/VendorUpdater.h"
#include "search_aggregator/search_aggregator.h"
#include "queue_manager/queue_manager.h"
#include "movie_source1/movie_source1.h"
// #include "movie_source2/movie_source2.h"  // paste movie_source2.h contents and I'll fill the line below in

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

// Checks .env next to the binary, then walks up from the binary dir looking
// for a repo-root .env. Falls back to std::getenv("TMDB_API_KEY").
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
//
// To add a new source:
//   1. #include "movie_sourceN/movie_sourceN.h" up top
//   2. add ONE line below:
//        aggregator->registerSource("Display Name", std::make_shared<ns::YourProvider>());
// That's it -- SearchAggregatorModule and SearchBridge handle the rest.
//
// Pass loadImages=false as a 3rd arg for a source you want to show as a
// fast, poster-free list (skips TMDB matching entirely for its results):
//   aggregator->registerSource("Some Source", std::make_shared<ns::Provider>(), /*loadImages=*/false);
// ---------------------------------------------------------------------
std::shared_ptr<search_aggregator::SearchAggregatorModule> buildSourceAggregator() {
    auto aggregator = std::make_shared<search_aggregator::SearchAggregatorModule>();

    aggregator->registerSource("Apibay (Torrent)", std::make_shared<movie_source1::ApibayProvider>());
    // aggregator->registerSource("<name>", std::make_shared<movie_source2::???Provider>());

    return aggregator;
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::cout << "Nothing Movies - We have what? Everything.\n";

#if defined(_WIN32)
    static const std::string kPlatformTag = "windows";
#else
    static const std::string kPlatformTag = "linux";
#endif

    static vendor_updater::VendorUpdater updater(
        "BunElysiaReact/nothing-browser",
        "vendor/nothing-browser",
        kPlatformTag
    );

    updater.startBackgroundWatch(6 * 3600, [](vendor_updater::UpdateResult r) {
        if (!r.error.empty()) {
            std::cerr << "[vendor_updater] error: " << r.error << "\n";
        } else if (r.updated) {
            std::cout << "[vendor_updater] updated " << r.oldTag << " -> " << r.newTag << "\n";
        }
    });

    const std::string tmdbApiKey = resolveTmdbApiKey();
    if (tmdbApiKey.empty()) {
        std::cerr << "[env] warning: TMDB_API_KEY not found (.env or process env) — "
                      "Trending/Upcoming carousels and search matching will fail to load\n";
    }

    ui::TmdbBridge tmdbBridge(tmdbApiKey);

    auto aggregator = buildSourceAggregator();
    ui::SearchBridge searchBridge(tmdbApiKey, aggregator);

    auto queueManager = std::make_shared<queue_manager::Queue_managerModule>();
    queueManager->init();

    ui::AppController appController;
    ui::QueueBridge queueBridge(queueManager);

    ui::MainWindow window(&tmdbBridge, &searchBridge, &appController, &queueBridge, queueManager);
    window.show();
    return app.exec();
}