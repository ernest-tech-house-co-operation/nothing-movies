#include "vendor_updater/VendorUpdater.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <zip.h>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace vendor_updater {

static size_t writeToString(void* c, size_t s, size_t n, void* out) {
    static_cast<std::string*>(out)->append((char*)c, s * n);
    return s * n;
}
static size_t writeToFile(void* c, size_t s, size_t n, void* stream) {
    return fwrite(c, s, n, static_cast<FILE*>(stream));
}

VendorUpdater::VendorUpdater(std::string repo, std::string vendorDir, std::string platformTag)
    : repo_(std::move(repo)), vendorDir_(std::move(vendorDir)), platformTag_(std::move(platformTag)) {
    fs::create_directories(vendorDir_);
}

VendorUpdater::~VendorUpdater() { stop(); }

std::string VendorUpdater::readLocalTag() const {
    std::ifstream f(vendorDir_ + "/.version");
    std::string tag;
    if (f) std::getline(f, tag);
    return tag;
}

void VendorUpdater::writeLocalTag(const std::string& tag) const {
    std::ofstream f(vendorDir_ + "/.version");
    f << tag;
}

UpdateResult VendorUpdater::checkAndUpdateOnce() {
    UpdateResult result{false, readLocalTag(), "", ""};

    CURL* curl = curl_easy_init();
    std::string body;
    if (curl) {
        std::string url = "https://api.github.com/repos/" + repo_ + "/releases/latest";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "nothingmovies-updater/1.0");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    json release;
    try {
        release = json::parse(body);
    } catch (...) {
        result.error = "Failed to parse release info";
        return result;
    }

    std::string latestTag = release.value("tag_name", "");
    result.newTag = latestTag;
    if (latestTag.empty() || latestTag == result.oldTag) return result;

    std::string assetUrl, assetName;
    for (auto& asset : release["assets"]) {
        std::string name = asset.value("name", "");
        bool matchesPlatform = name.find(platformTag_) != std::string::npos;
        bool isArchive = name.ends_with(".zip") || name.ends_with(".tar.gz");
        if (matchesPlatform && isArchive) {
            assetUrl = asset.value("browser_download_url", "");
            assetName = name;
            break;
        }
    }

    if (assetUrl.empty()) {
        result.error = "No matching asset for platform: " + platformTag_;
        return result;
    }

    std::string stagingDir = vendorDir_ + "_staging";
    fs::remove_all(stagingDir);
    fs::create_directories(stagingDir);
    std::string archivePath = stagingDir + "/" + assetName;

    curl = curl_easy_init();
    FILE* fp = fopen(archivePath.c_str(), "wb");
    if (!curl || !fp) {
        result.error = "Failed to open file/curl for download";
        return result;
    }
    curl_easy_setopt(curl, CURLOPT_URL, assetUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "nothingmovies-updater/1.0");
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        result.error = "Download failed";
        return result;
    }

    if (assetName.ends_with(".zip")) {
        int err = 0;
        zip_t* archive = zip_open(archivePath.c_str(), ZIP_RDONLY, &err);
        if (archive) {
            zip_int64_t numEntries = zip_get_num_entries(archive, 0);
            for (zip_int64_t i = 0; i < numEntries; ++i) {
                const char* name = zip_get_name(archive, i, 0);
                if (!name) continue;

                fs::path outPath = fs::path(stagingDir) / name;
                std::string nameStr(name);

                if (!nameStr.empty() && nameStr.back() == '/') {
                    fs::create_directories(outPath);
                    continue;
                }

                fs::create_directories(outPath.parent_path());
                zip_file_t* zf = zip_fopen_index(archive, i, 0);
                if (!zf) continue;

                std::ofstream out(outPath, std::ios::binary);
                char buf[8192];
                zip_int64_t bytesRead;
                while ((bytesRead = zip_fread(zf, buf, sizeof(buf))) > 0) {
                    out.write(buf, bytesRead);
                }
                zip_fclose(zf);
            }
            zip_close(archive);
        }
    }
    fs::remove(archivePath);

    std::string backupDir = vendorDir_ + "_prev";
    fs::remove_all(backupDir);
    if (fs::exists(vendorDir_)) fs::rename(vendorDir_, backupDir);
    fs::rename(stagingDir, vendorDir_);

    writeLocalTag(latestTag);
    fs::remove_all(backupDir);

    result.updated = true;
    return result;
}

void VendorUpdater::startBackgroundWatch(int intervalSeconds, std::function<void(UpdateResult)> onResult) {
    running_ = true;
    worker_ = std::thread([this, intervalSeconds, onResult]() {
        while (running_) {
            onResult(checkAndUpdateOnce());
            for (int i = 0; i < intervalSeconds && running_; ++i)
                std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}

void VendorUpdater::stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

}