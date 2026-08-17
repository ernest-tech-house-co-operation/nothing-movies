#pragma once
#include "vendor_updater/VendorUpdater.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace vendor_updater {

struct VendorInfo {
    std::string name;
    std::string sourceRepoUrl;
    std::string releaseRepo;
    std::string license;
    std::string currentTag;
    std::string vendorDir;
};

// The single place every movie source registers an external tool
// dependency with, instead of each source (or each part of the app)
// managing its own downloads however it likes.
//
// Two guarantees this makes, on purpose:
//
//  1. EVERYTHING LIVES IN ONE PLACE. Every registered vendor's files land
//     under rootDir()/<vendor name> — never anywhere else on disk. A
//     source cannot point its dependency at some arbitrary path; the
//     manager computes it.
//
//  2. NOTHING UNVERIFIABLE GETS REGISTERED. VendorSpec::sourceRepoUrl is
//     mandatory. It's the tool's own open-source repository — the thing
//     that lets anyone go read the code and confirm it isn't doing
//     anything it shouldn't (e.g. exfiltrating data). registerVendor()
//     refuses (and logs why) if it's missing. This is deliberately not
//     configurable — an environment variable or a "just this once" flag
//     defeats the entire point.
//
// Usage from a movie source (typically in a static registration block or
// early in your module's init()):
//
//     vendor_updater::VendorSpec spec;
//     spec.name              = "nothing-browser";
//     spec.sourceRepoUrl     = "https://github.com/ernest-tech-house-co-operation/nothing-browser";
//     spec.releaseRepo       = "BunElysiaReact/nothing-browser"; // where releases are actually published
//     spec.platformTag       = "linux-x86_64";
//     spec.assetMustContain  = {"headless"};
//     spec.license           = "MIT";
//     vendor_updater::VendorManager::instance().registerVendor(spec);
//
class VendorManager {
public:
    static VendorManager& instance();

    // Root folder every vendor tool is downloaded into, one subfolder per
    // vendor name. Override with the NOTHINGMOVIES_VENDOR_ROOT env var
    // (mainly for tests/dev); otherwise a fixed per-OS user data location
    // — this is the "Nothing folder" every vendor's files are confined to.
    std::string rootDir() const;

    // Returns false if:
    //   - spec.name is empty, already registered, or contains '/', '\', or ".."
    //     (which could otherwise be used to escape rootDir())
    //   - spec.sourceRepoUrl is empty (unverifiable — refused outright)
    //   - spec.releaseRepo and spec.releasesApiUrl are both empty
    //     (nothing to check updates against)
    // Reasons are logged to stderr on failure.
    bool registerVendor(const VendorSpec& spec);

    std::vector<VendorInfo> listVendors() const;

    // Manual, on-demand check for one vendor (used by the UI's "Check for
    // updates" button). Returns an error result if the name isn't registered.
    UpdateResult checkAndUpdate(const std::string& vendorName);

    // Non-owning pointer to the VendorUpdater backing a registered vendor.
    // For callers (like NothingBrowser) that need to drive their own
    // startup sequence — e.g. a synchronous first-fetch before spawning a
    // process — through the SAME instance VendorManager tracks, rather
    // than constructing a second, separately-owned one that could
    // silently diverge (this is exactly the bug that motivated this
    // method existing at all). Returns nullptr if name isn't registered.
    VendorUpdater* getUpdater(const std::string& name);

    // Starts a background watch thread per registered vendor.
    void startAllBackgroundWatches(int intervalSeconds, std::function<void(UpdateResult)> onResult);
    void stopAll();

    // Path to the manifest file listing every registered vendor's
    // declared source repo, release repo, and license — kept in plain
    // JSON inside rootDir() so it can be inspected independently of the
    // app's own UI, by anyone auditing what's been pulled in.
    std::string manifestPath() const;

private:
    VendorManager() = default;
    void writeManifest() const;
    bool isValidName(const std::string& name) const;

    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<VendorUpdater>> vendors_;
    std::map<std::string, VendorSpec> specs_;
};

} // namespace vendor_updater