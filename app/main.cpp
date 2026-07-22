#include <QApplication>
#include <QCoreApplication>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "ui/MainWindow.h"
#include "ui/TmdbBridge.h"
#include "vendor_updater/VendorUpdater.h"

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
                      "Trending/Upcoming carousels will fail to load\n";
    }

    ui::TmdbBridge tmdbBridge(tmdbApiKey);

    ui::MainWindow window(&tmdbBridge);
    window.show();
    return app.exec();
}