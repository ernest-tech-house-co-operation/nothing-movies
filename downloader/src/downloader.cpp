#include "downloader/downloader.h"

#include <curl/curl.h>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace downloader {

namespace {

size_t writeToFile(void* contents, size_t size, size_t nmemb, void* userp) {
    FILE* f = static_cast<FILE*>(userp);
    return fwrite(contents, size, nmemb, f);
}

} // namespace

struct DownloaderModule::Impl {
    struct Job {
        std::string id;
        std::string url;
        std::string destPath;
        std::thread worker;
        std::atomic<DownloadState> state{DownloadState::Pending};
        std::atomic<long long> downloadedBytes{0};
        std::atomic<long long> totalBytes{0};
        std::atomic<bool> cancelRequested{false};
    };

    // Progress callback lives here so it has access to the atomics without
    // needing curl to know about our Job struct directly.
    static int progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                 curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
        auto* job = static_cast<Job*>(clientp);
        job->totalBytes = dltotal;
        job->downloadedBytes = dlnow;
        return job->cancelRequested ? 1 : 0; // non-zero aborts the transfer
    }

    static void runJob(Job* job) {
        job->state = DownloadState::Downloading;

        FILE* file = fopen(job->destPath.c_str(), "wb");
        if (!file) {
            job->state = DownloadState::Error;
            return;
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            fclose(file);
            job->state = DownloadState::Error;
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, job->url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "nothingmovies/1.0");
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, job);

        const CURLcode res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        fclose(file);

        if (job->cancelRequested) {
            job->state = DownloadState::Cancelled;
        } else if (res != CURLE_OK) {
            job->state = DownloadState::Error;
        } else {
            job->state = DownloadState::Finished;
        }
    }

    mutable std::mutex mutex;
    std::unordered_map<std::string, std::unique_ptr<Job>> jobs;
    int nextId = 1;
};

DownloaderModule::DownloaderModule() : impl_(std::make_unique<Impl>()) {}
DownloaderModule::~DownloaderModule() {
    // Make sure no worker thread outlives the module.
    std::lock_guard<std::mutex> lock(impl_->mutex);
    for (auto& [id, job] : impl_->jobs) {
        job->cancelRequested = true;
        if (job->worker.joinable()) job->worker.join();
    }
}

void DownloaderModule::init() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

std::string DownloaderModule::startDownload(const std::string& url, const std::string& destPath) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    auto job = std::make_unique<Impl::Job>();
    const std::string id = "dl_" + std::to_string(impl_->nextId++);
    job->id = id;
    job->url = url;
    job->destPath = destPath;

    Impl::Job* rawJob = job.get();
    job->worker = std::thread(Impl::runJob, rawJob);

    impl_->jobs[id] = std::move(job);
    return id;
}

void DownloaderModule::update() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    for (auto& [id, job] : impl_->jobs) {
        const bool done = job->state == DownloadState::Finished ||
                           job->state == DownloadState::Error ||
                           job->state == DownloadState::Cancelled;
        if (done && job->worker.joinable()) {
            job->worker.join(); // reap it, thread object stays but is now inert
        }
    }
}

DownloadStatus DownloaderModule::getStatus(const std::string& id) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    DownloadStatus out;
    out.id = id;

    auto it = impl_->jobs.find(id);
    if (it == impl_->jobs.end()) {
        out.state = DownloadState::Error;
        return out;
    }

    const auto& job = it->second;
    out.url = job->url;
    out.destPath = job->destPath;
    out.state = job->state;
    out.downloadedBytes = job->downloadedBytes;
    out.totalBytes = job->totalBytes;
    out.progress = (out.totalBytes > 0)
        ? static_cast<double>(out.downloadedBytes) / static_cast<double>(out.totalBytes)
        : 0.0;

    return out;
}

void DownloaderModule::cancel(const std::string& id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto it = impl_->jobs.find(id);
    if (it != impl_->jobs.end()) {
        it->second->cancelRequested = true;
    }
}

} // namespace downloader