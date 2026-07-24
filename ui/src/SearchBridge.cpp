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

        // Clean every raw title locally first -- cheap, pure regex, no network.
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

        // Only fire a TMDB lookup for sources that want posters/matching
        // (SourceEntry::loadImages), and only once per unique (title, year)
        // pair, run in parallel -- this is what fixes N-sequential-calls
        // slowness. Sources with images disabled skip TMDB entirely and
        // fall straight through to the raw/cleaned-title list below.
        std::unordered_map<std::string, Cleaned> uniqueLookups;
        for (size_t i = 0; i < rawResults.size(); ++i) {
            if (!aggregator->isSourceImagesEnabled(rawResults[i].sourceName)) continue;
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

            QVariantMap entry;
            entry["id"] = QString::fromStdString(raw.id);
            entry["sourceName"] = QString::fromStdString(raw.sourceName);
            entry["rawTitle"] = QString::fromStdString(raw.title);

            auto matchIt = matchCache.find(keys[i]);
            const bool matched = matchIt != matchCache.end() && matchIt->second.found;
            entry["matched"] = matched;

            if (matched) {
                entry["title"] = QString::fromStdString(matchIt->second.officialTitle);
                entry["posterUrl"] = QString::fromStdString(matchIt->second.posterUrl);
                entry["year"] = matchIt->second.year;
            } else {
                // No TMDB match, or this source skips images -- fall back
                // to the locally-cleaned title, no poster, no network wait.
                entry["title"] = QString::fromStdString(cleaned[i].title);
                entry["posterUrl"] = QString::fromStdString(raw.posterUrl);
                entry["year"] = cleaned[i].year;
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
