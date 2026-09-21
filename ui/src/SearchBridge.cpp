#include "ui/SearchBridge.h"
#include <QMetaObject>
#include <QPointer>
#include <QVariantMap>
#include <thread>
#include <unordered_map>
SearchBridge::SearchBridge(const std::string& /*tmdbApiKey*/,
                           std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator,
                           QObject* parent)
    : QObject(parent), aggregator_(std::move(aggregator)) {}

void SearchBridge::search(const QString& query) {
    const std::string queryStd = query.toStdString();
    auto aggregator = aggregator_;
    QPointer<SearchBridge> self(this);

    std::thread([self, aggregator, queryStd]() {
        std::vector<core::MediaResult> rawResults;
        try {
            rawResults = aggregator->searchAll(queryStd);
        } catch (const std::exception& e) {
            if (!self) return;
            QMetaObject::invokeMethod(self, [self, message = QString::fromStdString(e.what())]() {
                if (self) emit self->searchError(message);
            }, Qt::QueuedConnection);
            return;
        }

        std::unordered_map<std::string, core::SourceCapabilities> capsCache;
        for (const auto& entry : aggregator->getAllSources())
            capsCache[entry.name] = entry.provider->getCapabilities();

        QVariantList results;
        for (const auto& raw : rawResults) {
            const std::string& srcName = raw.sourceName;
            auto capsIt = capsCache.find(srcName);
            bool hasInfo     = capsIt != capsCache.end() && capsIt->second.hasInfo;
            bool hasStream   = capsIt != capsCache.end() && capsIt->second.hasStream;
            bool hasDownload = capsIt != capsCache.end() && capsIt->second.hasDownload;
            std::string streamType   = capsIt != capsCache.end() ? capsIt->second.streamType   : "";
            std::string downloadType = capsIt != capsCache.end() ? capsIt->second.downloadType : "";

            QVariantMap entry;
            entry["id"]           = QString::fromStdString(raw.id);
            entry["sourceName"]   = QString::fromStdString(srcName);
            entry["rawTitle"]     = QString::fromStdString(raw.title);
            entry["title"]        = QString::fromStdString(raw.title);
            entry["type"]         = QString("movie");
            entry["posterUrl"]    = QString::fromStdString(raw.posterUrl);
            entry["year"]         = 0;
            entry["matched"]      = hasInfo;
            entry["hasInfo"]      = hasInfo;
            entry["hasStream"]    = hasStream;
            entry["hasDownload"]  = hasDownload;
            entry["streamType"]   = QString::fromStdString(streamType);
            entry["downloadType"] = QString::fromStdString(downloadType);
            results.append(entry);
        }

        if (!self) return;
        QMetaObject::invokeMethod(self, [self, results]() {
            if (self) emit self->resultsReady(results);
        }, Qt::QueuedConnection);
    }).detach();
}

void SearchBridge::searchTV(const QString& query) {
    const std::string queryStd = query.toStdString();
    auto aggregator = aggregator_;
    QPointer<SearchBridge> self(this);

    std::thread([self, aggregator, queryStd]() {
        std::vector<core::MediaResult> rawResults;
        try {
            rawResults = aggregator->searchAllTV(queryStd);
        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(self, [self, message = QString::fromStdString(e.what())]() {
                if (self) emit self->searchError(message);
            }, Qt::QueuedConnection);
            return;
        }

        QVariantList results;
        for (const auto& raw : rawResults) {
            QVariantMap entry;
            entry["id"]         = QString::fromStdString(raw.id);
            entry["sourceName"] = QString::fromStdString(raw.sourceName);
            entry["title"]      = QString::fromStdString(raw.title);
            entry["posterUrl"]  = QString::fromStdString(raw.posterUrl);
            entry["year"]       = raw.year;
            entry["type"]       = QString("tv");
            entry["hasInfo"]    = true;
            entry["hasStream"]  = true;
            entry["streamType"] = QString::fromStdString("http");
            results.append(entry);
        }

        QMetaObject::invokeMethod(self, [self, results]() {
            if (self) emit self->resultsReady(results);
        }, Qt::QueuedConnection);
    }).detach();
}

void SearchBridge::getDownloadOptions(const QString& title, int year, bool isTV) {
    auto aggregator = aggregator_;
    QPointer<SearchBridge> self(this);
    const std::string titleStd = title.toStdString();

    std::thread([self, aggregator, titleStd, year, isTV]() {
        QVariantList allOptions;
        for (const auto& entry : aggregator->getAllSources()) {
            const auto options = isTV
                ? entry.provider->getTVDownloadOptions(titleStd, year)
                : entry.provider->getDownloadOptions(titleStd, year);
            for (const auto& option : options) {
                QVariantMap item;
                item["magnetUrl"] = QString::fromStdString(option.magnetUrl);
                item["title"]     = QString::fromStdString(option.title);
                item["quality"]    = QString::fromStdString(option.quality);
                item["seeds"]      = option.seeds;
                item["size"]       = QString::fromStdString(option.size);
                allOptions.append(item);
            }
        }

        QMetaObject::invokeMethod(self, [self, allOptions]() {
            if (self) emit self->downloadOptionsReady(allOptions);
        }, Qt::QueuedConnection);
    }).detach();
}

void SearchBridge::getStreamUrl(const QString& id, const QString& sourceName, const QString& title) {
    auto aggregator = aggregator_;
    QPointer<SearchBridge> self(this);
    const std::string idStd         = id.toStdString();
    const std::string sourceNameStd = sourceName.toStdString();
    const QString titleCopy         = title;

    std::thread([self, aggregator, idStd, sourceNameStd, titleCopy]() {
        core::MediaResult result;
        result.id         = idStd;
        result.sourceName = sourceNameStd;

        std::string url;
        try {
            url = aggregator->getStreamUrl(result);
        } catch (const std::exception& e) {
            if (!self) return;
            QMetaObject::invokeMethod(self, [self, message = QString::fromStdString(e.what())]() {
                if (self) emit self->streamUrlError(message);
            }, Qt::QueuedConnection);
            return;
        }

        if (!self) return;

        if (url.empty()) {
            QMetaObject::invokeMethod(self, [self]() {
                if (self) emit self->streamUrlError("Could not resolve stream URL.");
            }, Qt::QueuedConnection);
            return;
        }

        const QString urlQt = QString::fromStdString(url);
        QMetaObject::invokeMethod(self, [self, titleCopy, urlQt]() {
            if (self) emit self->streamUrlReady(titleCopy, urlQt);
        }, Qt::QueuedConnection);
    }).detach();
}
 // namespace ui
