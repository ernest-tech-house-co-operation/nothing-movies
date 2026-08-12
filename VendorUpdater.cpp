#include "vendor_updater/VendorUpdater.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <zip.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace vendor_updater {

namespace {

size_t writeToString(void* c, size_t s, size_t n, void* out) {
    static_cast<std::string*>(out)->append((char*)c, s * n);
    return s * n;
}
size_t writeToFile(void* c, size_t s, size_t n, void* stream) {
    return fwrite(c, s, n, static_cast<FILE*>(stream));
}

bool extractZip(const std::string& archivePath, const std::string& stagingDir) {
    int err = 0;
    zip_t* archive = zip_open(archivePath.c_str(), ZIP_RDONLY, &err);
    if (!archive) return false;

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

        // Release assets ship executables without exec bit set once
        // extracted from an archive that didn't preserve it - libzip
        // doesn't restore unix permissions from the zip's external
        // attributes here, so make sure the binary is runnable.
        fs::permissions(outPath,
                         fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
                             fs::perms::others_read | fs::perms::others_exec,
                         fs::perm_options::add);
    }
    zip_close(archive);
    return true;
}

bool extractTarGz(const std::string& archivePath, const std::string& stagingDir) {
    // --strip-components=1: GitHub release tarballs wrap their contents in
    // a top-level folder (e.g. "toolname-0.2.0-linux-x86_64/"). Without
    // stripping it, the binary ends up nested one level deeper than the
    // vendor dir the rest of the app expects.
    std::string cmd = "tar -xzf \"" + archivePath + "\" -C \"" + stagingDir + "\" --strip-components=1";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "[vendor_updater] tar extraction failed (exit " << rc << "): " << cmd << "\n";
        return false;
    }
    return true;
}

// Generalized replacement for the old hardcoded "isHeadless" check — a
// vendor spec now declares whatever required/excluded substrings it needs
// to pick the right asset out of a release that may publish several
// builds sharing the same platform suffix.
bool assetMatches(const std::string& assetName, const VendorSpec& spec) {
    if (spec.platformTag.empty() || assetName.find(spec.platformTag) == std::string::npos)
        return false;

    for (const auto& required : spec.assetMustContain) {
        if (assetName.find(required) == std::string::npos) return false;
    }
    for (const auto& excluded : spec.assetMustNotContain) {
        if (!excluded.empty() && assetName.find(excluded) != std::string::npos) return false;
    }

    bool isArchive = assetName.ends_with(".zip") || assetName.ends_with(".tar.gz");
    return isArchive;
}

} // namespace

VendorUpdater::VendorUpdater(VendorSpec spec, std::string vendorDir)
    : spec_(std::move(spec)), vendorDir_(std::move(vendorDir)) {
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

std::string VendorUpdater::resolveReleasesApiUrl() const {
    if (!spec_.releasesApiUrl.empty()) return spec_.releasesApiUrl;
    return "https://api.github.com/repos/" + spec_.releaseRepo + "/releases/latest";
}

UpdateResult VendorUpdater::checkAndUpdateOnce() {
    UpdateResult result;
    result.vendorName = spec_.name;
    result.oldTag = readLocalTag();

    std::cout << "[vendor_updater] [" << spec_.name << "] checking (local tag: "
               << (result.oldTag.empty() ? "<none>" : result.oldTag) << ")\n";

    CURL* curl = curl_easy_init();
    std::string body;
    if (curl) {
        std::string url = resolveReleasesApiUrl();
        std::cout << "[vendor_updater] [" << spec_.name << "] fetching release info: " << url << "\n";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "nothingmovies-updater/1.0");
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    } else {
        result.error = "curl_easy_init failed";
        return result;
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
    std::cout << "[vendor_updater] [" << spec_.name << "] latest release tag: "
               << (latestTag.empty() ? "<none>" : latestTag) << "\n";

    if (latestTag.empty() || latestTag == result.oldTag) {
        std::cout << "[vendor_updater] [" << spec_.name << "] already up to date — skipping fetch\n";
        return result;
    }

    std::string assetUrl, assetName;
    for (auto& asset : release["assets"]) {
        std::string name = asset.value("name", "");
        if (assetMatches(name, spec_)) {
            assetUrl = asset.value("browser_download_url", "");
            assetName = name;
            break;
        }
    }

    if (assetUrl.empty()) {
        result.error = "No matching asset for platform: " + spec_.platformTag;
        return result;
    }
    std::cout << "[vendor_updater] [" << spec_.name << "] matched asset: " << assetName << "\n";
    std::cout << "[vendor_updater] [" << spec_.name << "] downloading from: " << assetUrl << "\n";

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
        result.error = std::string("Download failed: ") + curl_easy_strerror(res);
        fs::remove_all(stagingDir);
        return result;
    }

    std::error_code sizeEc;
    auto downloadedBytes = fs::file_size(archivePath, sizeEc);
    std::cout << "[vendor_updater] [" << spec_.name << "] downloaded " << archivePath
               << " (" << (sizeEc ? "unknown size" : std::to_string(downloadedBytes) + " bytes") << ")\n";

    std::cout << "[vendor_updater] [" << spec_.name << "] extracting " << assetName << " -> " << stagingDir << "\n";
    bool extracted = false;
    if (assetName.ends_with(".zip")) {
        extracted = extractZip(archivePath, stagingDir);
    } else if (assetName.ends_with(".tar.gz")) {
        extracted = extractTarGz(archivePath, stagingDir);
    }
    std::cout << "[vendor_updater] [" << spec_.name << "] extraction " << (extracted ? "succeeded" : "FAILED") << "\n";

    std::cout << "[vendor_updater] [" << spec_.name << "] deleting downloaded archive: " << archivePath << "\n";
    fs::remove(archivePath);

    if (!extracted) {
        result.error = "Failed to extract asset: " + assetName;
        fs::remove_all(stagingDir);
        return result;
    }

    if (fs::is_empty(stagingDir)) {
        result.error = "Extraction produced no files: " + assetName;
        std::cout << "[vendor_updater] [" << spec_.name << "] staging dir is empty after extraction — aborting swap\n";
        fs::remove_all(stagingDir);
        return result;
    }

    std::string backupDir = vendorDir_ + "_prev";
    std::cout << "[vendor_updater] [" << spec_.name << "] placing " << stagingDir << " -> " << vendorDir_ << "\n";
    fs::remove_all(backupDir);
    if (fs::exists(vendorDir_)) fs::rename(vendorDir_, backupDir);
    fs::rename(stagingDir, vendorDir_);

    writeLocalTag(latestTag);
    fs::remove_all(backupDir);

    std::cout << "[vendor_updater] [" << spec_.name << "] done — now at " << latestTag << "\n";
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

} // namespace vendor_updater
