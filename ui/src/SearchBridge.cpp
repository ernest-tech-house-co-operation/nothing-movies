#include "ui/SearchBridge.h"
#include "tmdb_client/TitleCleaner.h"

#include <QMetaObject>
#include <QPointer>
#include <QVariantMap>
#include <future>
#include <thread>
#include <unordered_map>

namespace ui {

SearchBridge::SearchBridge(const std::string& tmdbApiKey,
                           std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator,
                           QObject* parent)
    : QObject(parent),
      aggregator_(std::move(aggregator)),
      tmdbClient_(std::make_shared<tmdb_client::TmdbClient>(tmdbApiKey)) {}

void SearchBridge::search(const QString& query) {
    const std::string queryStd = query.toStdString();
    auto aggregator = aggregator_;
    auto tmdbClient = tmdbClient_;
    QPointer<SearchBridge> self(this);

    std::thread([self, aggregator, tmdbClient, queryStd]() {
        std::vector<core::MediaResult> rawResults;
        try {
            rawResults = aggregator->searchAll(queryStd);
        } catch (const std::exception& e) {
            if (!self) return;
            QMetaObject::invokeMethod(
                self,
                [self, message = QString::fromStdString(e.what())]() {
                    if (self) emit self->searchError(message);
                },
                Qt::QueuedConnection);
            return;
        }

        // Build a capability map per source name so we don't call
        // getCapabilities() repeatedly inside the loop
        std::unordered_map<std::string, core::SourceCapabilities> capsCache;
        for (const auto& entry : aggregator->getAllSources()) {
            capsCache[entry.name] = entry.provider->getCapabilities();
        }

        // Clean every raw title locally first — cheap, pure regex, no network.
        struct Cleaned {
            std::string title;
            int year = 0;
        };
        std::vector<Cleaned> cleaned(rawResults.size());
        std::vector<std::string> keys(rawResults.size());
        for (size_t i = 0; i < rawResults.size(); ++i) {
            const auto c = tmdb_client::TitleCleaner::clean(rawResults[i].title);
            cleaned[i] = {c.title, c.year};
            keys[i] = c.title + "|" + std::to_string(c.year);
        }

        // Only fire TMDB lookups for sources where:
        //   - loadImages is true (source wants posters)
        //   - hasInfo is false (source doesn't provide its own clean data)
        // Sources with hasInfo=true already return clean titles+posters
        // from the source itself — running them through TMDB is redundant
        // and slow.
        std::unordered_map<std::string, Cleaned> uniqueLookups;
        for (size_t i = 0; i < rawResults.size(); ++i) {
            const std::string& srcName = rawResults[i].sourceName;
            auto capsIt = capsCache.find(srcName);
            bool hasInfo = capsIt != capsCache.end() && capsIt->second.hasInfo;
            if (hasInfo) continue;  // skip TMDB — source provides its own data
            if (!aggregator->isSourceImagesEnabled(srcName)) continue;
            uniqueLookups.emplace(keys[i], cleaned[i]);
        }

        std::vector<std::string> uniqueKeys;
        std::vector<std::future<tmdb_client::TmdbMatch>> futures;
        uniqueKeys.reserve(uniqueLookups.size());
        futures.reserve(uniqueLookups.size());
        for (const auto& kv : uniqueLookups) {
            uniqueKeys.push_back(kv.first);
            const Cleaned c = kv.second;
            futures.push_back(std::async(std::launch::async, [tmdbClient, c]() {
                return tmdbClient->searchBestMatch(c.title, c.year);
            }));
        }

        std::unordered_map<std::string, tmdb_client::TmdbMatch> matchCache;
        matchCache.reserve(uniqueKeys.size());
        for (size_t i = 0; i < uniqueKeys.size(); ++i) {
            matchCache[uniqueKeys[i]] = futures[i].get();
        }

        QVariantList results;
        results.reserve(static_cast<int>(rawResults.size()));
        for (size_t i = 0; i < rawResults.size(); ++i) {
            const auto& raw = rawResults[i];
            const std::string& srcName = raw.sourceName;

            auto capsIt = capsCache.find(srcName);
            bool hasInfo = capsIt != capsCache.end() && capsIt->second.hasInfo;
            bool hasStream = capsIt != capsCache.end() && capsIt->second.hasStream;
            bool hasDownload = capsIt != capsCache.end() && capsIt->second.hasDownload;
            std::string streamType = capsIt != capsCache.end() ? capsIt->second.streamType : "";
            std::string downloadType = capsIt != capsCache.end() ? capsIt->second.downloadType : "";

            QVariantMap entry;
            entry["id"]           = QString::fromStdString(raw.id);
            entry["sourceName"]   = QString::fromStdString(srcName);
            entry["rawTitle"]     = QString::fromStdString(raw.title);
            entry["hasInfo"]      = hasInfo;
            entry["hasStream"]    = hasStream;
            entry["hasDownload"]  = hasDownload;
            entry["streamType"]   = QString::fromStdString(streamType);
            entry["downloadType"] = QString::fromStdString(downloadType);

            if (hasInfo) {
                // Source provides its own clean data — use it directly,
                // no TMDB involved at all
                entry["matched"]   = true;
                entry["title"]     = QString::fromStdString(raw.title);
                entry["posterUrl"] = QString::fromStdString(raw.posterUrl);
                entry["year"]      = 0;  // sources with hasInfo return year via getMediaInfo
            } else {
                auto matchIt = matchCache.find(keys[i]);
                const bool matched = matchIt != matchCache.end() && matchIt->second.found;
                entry["matched"] = matched;

                if (matched) {
                    entry["title"]     = QString::fromStdString(matchIt->second.officialTitle);
                    entry["posterUrl"] = QString::fromStdString(matchIt->second.posterUrl);
                    entry["year"]      = matchIt->second.year;
                } else {
                    entry["title"]     = QString::fromStdString(cleaned[i].title);
                    entry["posterUrl"] = QString::fromStdString(raw.posterUrl);
                    entry["year"]      = cleaned[i].year;
                }
            }

            results.append(entry);
        }

        if (!self) return;
        QMetaObject::invokeMethod(
            self,
            [self, results]() {
                if (self) emit self->resultsReady(results);
            },
            Qt::QueuedConnection);
    }).detach();
}

void SearchBridge::getStreamUrl(const QString& id, const QString& sourceName, const QString& title) {
    auto aggregator = aggregator_;
    QPointer<SearchBridge> self(this);
    const std::string idStd = id.toStdString();
    const std::string sourceNameStd = sourceName.toStdString();
    const QString titleCopy = title;

    std::thread([self, aggregator, idStd, sourceNameStd, titleCopy]() {
        core::MediaResult result;
        result.id = idStd;
        result.sourceName = sourceNameStd;

        std::string url;
        try {
            url = aggregator->getStreamUrl(result);
        } catch (const std::exception& e) {
            if (!self) return;
            QMetaObject::invokeMethod(
                self,
                [self, message = QString::fromStdString(e.what())]() {
                    if (self) emit self->streamUrlError(message);
                },
                Qt::QueuedConnection);
            return;
        }

        if (!self) return;

        if (url.empty()) {
            QMetaObject::invokeMethod(
                self,
                [self]() {
                    if (self) emit self->streamUrlError("Could not resolve a stream URL for this source.");
                },
                Qt::QueuedConnection);
            return;
        }

        const QString urlQt = QString::fromStdString(url);
        QMetaObject::invokeMethod(
            self,
            [self, titleCopy, urlQt]() {
                if (self) emit self->streamUrlReady(titleCopy, urlQt);
            },
            Qt::QueuedConnection);
    }).detach();
}

}  // namespace ui