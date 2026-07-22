#pragma once
#include <string>
#include <functional>
#include <atomic>
#include <thread>

namespace vendor_updater {

struct UpdateResult {
    bool updated;
    std::string oldTag;
    std::string newTag;
    std::string error;
};

class VendorUpdater {
public:
    VendorUpdater(std::string repo, std::string vendorDir, std::string platformTag);
    ~VendorUpdater();

    UpdateResult checkAndUpdateOnce();
    void startBackgroundWatch(int intervalSeconds, std::function<void(UpdateResult)> onResult);
    void stop();

private:
    std::string readLocalTag() const;
    void writeLocalTag(const std::string& tag) const;

    std::string repo_;
    std::string vendorDir_;
    std::string platformTag_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

}
