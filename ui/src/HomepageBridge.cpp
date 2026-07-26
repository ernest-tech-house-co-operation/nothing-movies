#include "ui/HomepageBridge.h"
#include "ui/TmdbBridge.h"
#include "core/ISourceProvider.h"
#include "search_aggregator/search_aggregator.h"

#include <QVariant>
#include <QDebug>

namespace ui {

HomepageBridge::HomepageBridge(std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator,
                               TmdbBridge* tmdbBridge,
                               QObject* parent)
    : QObject(parent), aggregator_(aggregator), tmdbBridge_(tmdbBridge)
{
}

void HomepageBridge::loadHomepage() {
    if (!aggregator_) {
        emit homepageError("Aggregator not available");
        return;
    }

    auto sources = aggregator_->getAllSources();

    for (const auto& entry : sources) {
        auto caps = entry.provider->getCapabilities();
        if (caps.hasHomepage) {
            auto items = entry.provider->getHomepage();
            QVariantList qmlItems;
            for (const auto& item : items) {
                QVariantMap map;
                map["id"]           = QString::fromStdString(item.id);
                map["title"]        = QString::fromStdString(item.title);
                map["posterUrl"]    = QString::fromStdString(item.imageUrl);
                map["category"]     = QString::fromStdString(item.category);
                map["trailerLink"]  = QString::fromStdString(item.trailerLink);
                map["genre"]        = QString::fromStdString(item.genre);
                map["rating"]       = item.rating;
                map["hasInfo"]      = caps.hasInfo;
                map["hasStream"]    = caps.hasStream;
                map["hasDownload"]  = caps.hasDownload;
                map["streamType"]   = QString::fromStdString(caps.streamType);
                map["downloadType"] = QString::fromStdString(caps.downloadType);
                map["sourceName"]   = QString::fromStdString(entry.name);
                qmlItems.append(map);
            }
            emit homepageReady(qmlItems, QString::fromStdString(entry.name), false);
            return;
        }
    }

    // No source has homepage → fallback to TMDB trending
    if (tmdbBridge_) {
        auto* bridge = tmdbBridge_;
        connect(bridge, &TmdbBridge::trendingReady, this, [this, bridge](QVariantList movies) {
            disconnect(bridge, &TmdbBridge::trendingReady, this, nullptr);
            emit homepageReady(movies, "TMDB Fallback", true);
        });
        bridge->loadTrending();
    } else {
        emit homepageError("No TMDB bridge available for fallback");
    }
}

void HomepageBridge::loadInfo(const QString& id, const QString& sourceName) {
    if (!aggregator_) {
        emit infoError("Aggregator not available");
        return;
    }

    auto sources = aggregator_->getAllSources();

    for (const auto& entry : sources) {
        if (QString::fromStdString(entry.name) != sourceName) continue;

        auto caps = entry.provider->getCapabilities();
        if (!caps.hasInfo) {
            emit infoError(sourceName + " does not support info pages");
            return;
        }

        auto info = entry.provider->getMediaInfo(id.toStdString());

        QVariantMap map;
        map["id"]          = QString::fromStdString(info.id);
        map["title"]       = QString::fromStdString(info.title);
        map["type"]        = QString::fromStdString(info.type);
        map["posterUrl"]   = QString::fromStdString(info.posterUrl);
        map["backdropUrl"] = QString::fromStdString(info.backdropUrl);
        map["synopsis"]    = QString::fromStdString(info.synopsis);
        map["trailerUrl"]  = QString::fromStdString(info.trailerUrl);
        map["quality"]     = QString::fromStdString(info.quality);
        map["year"]        = info.year;
        map["lengthMins"]  = info.lengthMins;
        map["rating"]      = info.rating;
        map["sourceName"]  = sourceName;

        map["hasStream"]    = caps.hasStream;
        map["hasDownload"]  = caps.hasDownload;
        map["streamType"]   = QString::fromStdString(caps.streamType);
        map["downloadType"] = QString::fromStdString(caps.downloadType);
        map["hasCast"]      = caps.info.hasCast;
        map["hasSynopsis"]  = caps.info.hasSynopsis;
        map["hasTrailer"]   = caps.info.hasTrailer;
        map["hasQuality"]   = caps.info.hasQuality;
        map["hasLength"]    = caps.info.hasLength;
        map["hasYear"]      = caps.info.hasYear;
        map["hasRating"]    = caps.info.hasRating;

        QVariantList castList;
        for (const auto& actor : info.cast)
            castList.append(QString::fromStdString(actor));
        map["cast"] = castList;

        QVariantList episodeList;
        for (const auto& ep : info.episodes) {
            QVariantMap epMap;
            epMap["season"]    = ep.season;
            epMap["episode"]   = ep.episode;
            epMap["title"]     = QString::fromStdString(ep.title);
            epMap["synopsis"]  = QString::fromStdString(ep.synopsis);
            epMap["streamUrl"] = QString::fromStdString(ep.streamUrl);
            episodeList.append(epMap);
        }
        map["episodes"] = episodeList;

        emit infoReady(map);
        return;
    }

    emit infoError("Source not found: " + sourceName);
}

QVariantList HomepageBridge::getSources() {
    QVariantList list;
    if (!aggregator_) return list;

    for (const auto& entry : aggregator_->getAllSources()) {
        auto caps = entry.provider->getCapabilities();
        QVariantMap map;
        map["name"]           = QString::fromStdString(entry.name);
        map["version"]        = QString::fromStdString(caps.version);
        map["profilePicture"] = QString::fromStdString(caps.profilePicture);
        map["hasHomepage"]    = caps.hasHomepage;
        map["hasInfo"]        = caps.hasInfo;
        map["hasStream"]      = caps.hasStream;
        map["hasDownload"]    = caps.hasDownload;
        map["streamType"]     = QString::fromStdString(caps.streamType);
        map["downloadType"]   = QString::fromStdString(caps.downloadType);
        list.append(map);
    }

    return list;
}
QVariantList HomepageBridge::getSubtitleUrls(const QString& id, const QString& sourceName) {
    QVariantList list;
    if (!aggregator_) return list;
    for (const auto& entry : aggregator_->getAllSources()) {
        if (QString::fromStdString(entry.name) != sourceName) continue;
        auto urls = entry.provider->getSubtitleUrls(id.toStdString());
        for (const auto& url : urls)
            list.append(QString::fromStdString(url));
        break;
    }
    return list;
}

} // namespace ui