#include "vendor_updater/VendorManager.h"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace vendor_updater {

VendorManager& VendorManager::instance() {
    static VendorManager mgr;
    return mgr;
}

std::string VendorManager::rootDir() const {
    if (const char* override = std::getenv("NOTHINGMOVIES_VENDOR_ROOT")) {
        return std::string(override);
    }
#ifdef _WIN32
    const char* localAppData = std::getenv("LOCALAPPDATA");
    std::string base = localAppData ? localAppData : ".";
    return base + "\\NothingMovies\\vendor";
#else
    const char* home = std::getenv("HOME");
    std::string base = home ? home : ".";
    return base + "/.local/share/nothingmovies/vendor";
#endif
}

bool VendorManager::isValidName(const std::string& name) const {
    if (name.empty()) return false;
    if (name.find('/') != std::string::npos) return false;
    if (name.find('\\') != std::string::npos) return false;
    if (name.find("..") != std::string::npos) return false;
    return true;
}

bool VendorManager::registerVendor(const VendorSpec& spec) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isValidName(spec.name)) {
        std::cerr << "[VendorManager] Refusing to register vendor with invalid name: \""
                   << spec.name << "\" (must be non-empty and contain no '/', '\\', or \"..\")\n";
        return false;
    }

    if (specs_.count(spec.name)) {
        std::cerr << "[VendorManager] Vendor \"" << spec.name << "\" is already registered — skipping\n";
        return false;
    }

    if (spec.sourceRepoUrl.empty()) {
        std::cerr << "[VendorManager] Refusing to register \"" << spec.name
                   << "\": no sourceRepoUrl given. Every vendor tool must declare its own "
                   << "open-source repository so what gets downloaded and run can be verified. "
                   << "Set VendorSpec::sourceRepoUrl and try again.\n";
        return false;
    }

    if (spec.releaseRepo.empty() && spec.releasesApiUrl.empty()) {
        std::cerr << "[VendorManager] Refusing to register \"" << spec.name
                   << "\": neither releaseRepo nor releasesApiUrl is set — nothing to check "
                   << "updates against.\n";
        return false;
    }

    std::string vendorDir = rootDir() + "/" + spec.name;
    vendors_[spec.name] = std::make_unique<VendorUpdater>(spec, vendorDir);
    specs_[spec.name] = spec;

    std::cout << "[VendorManager] Registered vendor \"" << spec.name << "\" -> " << vendorDir
               << " (source: " << spec.sourceRepoUrl << ")\n";

    writeManifest();
    return true;
}

std::vector<VendorInfo> VendorManager::listVendors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<VendorInfo> result;
    for (const auto& [name, updater] : vendors_) {
        const auto& spec = specs_.at(name);
        VendorInfo info;
        info.name = name;
        info.sourceRepoUrl = spec.sourceRepoUrl;
        info.releaseRepo = spec.releaseRepo;
        info.license = spec.license;
        info.currentTag = updater->currentTag();
        info.vendorDir = updater->vendorDir();
        result.push_back(info);
    }
    return result;
}

UpdateResult VendorManager::checkAndUpdate(const std::string& vendorName) {
    VendorUpdater* updater = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = vendors_.find(vendorName);
        if (it == vendors_.end()) {
            UpdateResult result;
            result.vendorName = vendorName;
            result.error = "Unknown vendor: " + vendorName;
            return result;
        }
        updater = it->second.get();
    }
    // Deliberately called outside the lock — checkAndUpdateOnce() does
    // network I/O and file extraction, and shouldn't hold up other
    // vendors' registration/listing calls while it runs.
    return updater->checkAndUpdateOnce();
}

void VendorManager::startAllBackgroundWatches(int intervalSeconds, std::function<void(UpdateResult)> onResult) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [name, updater] : vendors_) {
        updater->startBackgroundWatch(intervalSeconds, onResult);
    }
}

void VendorManager::stopAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [name, updater] : vendors_) {
        updater->stop();
    }
}

std::string VendorManager::manifestPath() const {
    return rootDir() + "/manifest.json";
}

void VendorManager::writeManifest() const {
    // Note: called while mutex_ is already held by registerVendor().
    fs::create_directories(rootDir());

    json manifest = json::array();
    for (const auto& [name, spec] : specs_) {
        json entry;
        entry["name"] = spec.name;
        entry["sourceRepoUrl"] = spec.sourceRepoUrl;
        entry["releaseRepo"] = spec.releaseRepo;
        entry["releasesApiUrl"] = spec.releasesApiUrl;
        entry["platformTag"] = spec.platformTag;
        entry["license"] = spec.license;
        manifest.push_back(entry);
    }

    std::ofstream f(manifestPath());
    f << manifest.dump(2);
}

} // namespace vendor_updater
