#pragma once
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>

namespace vendor_updater {

struct UpdateResult {
    bool updated = false;
    std::string oldTag;
    std::string newTag;
    std::string error;
    std::string vendorName; // which vendor this result belongs to (multi-vendor aware)
};

// Describes one external tool a movie source depends on. This is the only
// thing a source needs to hand VendorManager — everything about where the
// tool lives, where updates come from, and how to identify the right
// release asset is captured here instead of being hardcoded per-tool.
struct VendorSpec {
    // Unique id. Also becomes this vendor's folder name under the shared
    // vendor root — must not contain '/', '\', or "..".
    std::string name;

    // REQUIRED. Link to the tool's own open-source repository (e.g.
    // "https://github.com/org/tool"). This is what lets anyone — a
    // reviewer, a user, an auditor — go verify exactly what code is being
    // downloaded and executed. VendorManager::registerVendor() refuses
    // registration outright if this is empty; an unverifiable binary
    // dependency doesn't get silently trusted.
    std::string sourceRepoUrl;

    // "owner/repo" used to build the GitHub releases API URL
    // (https://api.github.com/repos/<releaseRepo>/releases/latest), used
    // when releasesApiUrl is left empty. Usually the same project as
    // sourceRepoUrl, but kept separate in case releases are published
    // from a different repo (e.g. a build/dist repo).
    std::string releaseRepo;

    // Optional full override for tools that don't publish releases via
    // the GitHub API in the standard way. If set, this is used verbatim
    // instead of building a URL from releaseRepo.
    std::string releasesApiUrl;

    // Substring identifying this platform's release asset, e.g.
    // "linux-x86_64" or "win-x64".
    std::string platformTag;

    // Extra substrings the asset filename must contain to disambiguate
    // between multiple builds sharing the same platform suffix (e.g.
    // "headless" vs a full GUI build).
    std::vector<std::string> assetMustContain;

    // Substrings that disqualify an asset even if platformTag/assetMustContain match.
    std::vector<std::string> assetMustNotContain;

    // Informational only — shown in the UI (e.g. "MIT", "GPLv2").
    std::string license;
};

class VendorUpdater {
public:
    // vendorDir is computed and owned by VendorManager (always
    // <vendor root>/<spec.name>) — never pass an arbitrary path directly.
    VendorUpdater(VendorSpec spec, std::string vendorDir);
    ~VendorUpdater();

    UpdateResult checkAndUpdateOnce();

    void startBackgroundWatch(int intervalSeconds, std::function<void(UpdateResult)> onResult);
    void stop();

    const VendorSpec& spec() const { return spec_; }
    const std::string& vendorDir() const { return vendorDir_; }
    std::string currentTag() const { return readLocalTag(); }

private:
    std::string readLocalTag() const;
    void writeLocalTag(const std::string& tag) const;
    std::string resolveReleasesApiUrl() const;

    VendorSpec spec_;
    std::string vendorDir_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

} // namespace vendor_updater
