#include "ui/HomepageBridge.h"
#include "core/ISourceProvider.h"
#include "search_aggregator/search_aggregator.h"
#include <QVariant>
#include <QMetaObject>
#include <QPointer>
#include <thread>
#include <iostream>

HomepageBridge::HomepageBridge(std::shared_ptr<search_aggregator::SearchAggregatorModule> aggregator,
                               std::nullptr_t,
                               QObject* parent)
    : QObject(parent), aggregator_(aggregator) {}

void HomepageBridge::loadHomepage() {
    if (!aggregator_) { emit homepageError("Aggregator not available"); return; }

    auto aggregator = aggregator_;
    QPointer<HomepageBridge> self(this);

    std::thread([self, aggregator]() {
        for (const auto& entry : aggregator->getAllSources()) {
            auto caps = entry.provider->getCapabilities();
            std::cerr << "[HomepageBridge] checking source: " << entry.name
                      << " hasHomepage=" << caps.hasHomepage << std::endl;
            if (!caps.hasHomepage) continue;

            auto items = entry.provider->getHomepage();
            std::cerr << "[HomepageBridge] got " << items.size() << " items from " << entry.name << std::endl;

            QVariantList qmlItems;
            for (const auto& item : items) {
                QVariantMap map;
                map["id"]           = QString::fromStdString(item.id);
                map["title"]        = QString::fromStdString(item.title);
                map["posterUrl"]    = QString::fromStdString(item.imageUrl);
                map["backdropUrl"]  = QString::fromStdString(item.backdropUrl);
                map["category"]     = QString::fromStdString(item.category);
                map["trailerLink"]  = QString::fromStdString(item.trailerLink);
                map["genre"]        = QString::fromStdString(item.genre);
                map["rating"]       = item.rating;
                map["year"]         = item.year;
                map["hasInfo"]      = caps.hasInfo;
                map["hasStream"]    = caps.hasStream;
                map["hasDownload"]  = caps.hasDownload;
                map["streamType"]   = QString::fromStdString(caps.streamType);
                map["downloadType"] = QString::fromStdString(caps.downloadType);
                map["sourceName"]   = QString::fromStdString(entry.name);
                qmlItems.append(map);
            }

            const QString sourceName = QString::fromStdString(entry.name);
            std::cerr << "[HomepageBridge] invoking homepageReady with " << qmlItems.size() << " items" << std::endl;
            QMetaObject::invokeMethod(self, [self, qmlItems, sourceName]() {
                if (self) {
                    std::cerr << "[HomepageBridge] emitting signal on main thread" << std::endl;
                    emit self->homepageReady(qmlItems, sourceName, false);
                } else {
                    std::cerr << "[HomepageBridge] self is null!" << std::endl;
                }
            }, Qt::QueuedConnection);
            return;
        }

        QMetaObject::invokeMethod(self, [self]() {
            if (self) emit self->homepageError("No source with homepage available");
        }, Qt::QueuedConnection);
    }).detach();
}

void HomepageBridge::loadInfo(const QString& id, const QString& sourceName, const QString& type) {
    if (!aggregator_) { emit infoError("Aggregator not available"); return; }

    auto aggregator = aggregator_;
    QPointer<HomepageBridge> self(this);
    const std::string idStd  = id.toStdString();
    const std::string srcStd = sourceName.toStdString();

    std::thread([self, aggregator, idStd, srcStd, sourceName, type]() {
        for (const auto& entry : aggregator->getAllSources()) {
            if (entry.name != srcStd) continue;
            auto caps = entry.provider->getCapabilities();
            if (!caps.hasInfo) {
                QMetaObject::invokeMethod(self, [self, sourceName]() {
                    if (self) emit self->infoError(sourceName + " does not support info pages");
                }, Qt::QueuedConnection);
                return;
            }

            auto info = type == "tv"
                            ? entry.provider->getTVInfo(idStd)
                            : entry.provider->getMediaInfo(idStd);
            QVariantMap map;
            map["id"]          = QString::fromStdString(info.id);
            map["title"]       = QString::fromStdString(info.title);
            map["type"]        = QString::fromStdString(info.type);
            map["posterUrl"]   = QString::fromStdString(info.posterUrl);
            map["backdropUrl"] = QString::fromStdString(info.backdropUrl);
            map["synopsis"]    = QString::fromStdString(info.synopsis);
            map["trailerUrl"]  = QString::fromStdString(info.trailerUrl);
            map["trailerKey"]  = QString::fromStdString(info.trailerKey);
            map["quality"]     = QString::fromStdString(info.quality);
            map["imdbId"]      = QString::fromStdString(info.imdbId);
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
            map["hasSoundtrack"] = caps.info.hasSoundtrack;
            map["hasSimilar"]   = caps.info.hasSimilar;

            QVariantList castList;
            for (const auto& actor : info.cast) {
                QVariantMap m;
                m["name"]       = QString::fromStdString(actor.name);
                m["character"]  = QString::fromStdString(actor.character);
                m["profileUrl"] = QString::fromStdString(actor.profileUrl);
                castList.append(m);
            }
            map["cast"] = castList;

            QVariantList similarList;
            for (const auto& sim : info.similar) {
                QVariantMap m;
                m["id"]        = QString::fromStdString(sim.id);
                m["title"]     = QString::fromStdString(sim.title);
                m["posterUrl"] = QString::fromStdString(sim.posterUrl);
                similarList.append(m);
            }
            map["similar"] = similarList;

            QVariantList soundtrackList;
            for (const auto& t : info.soundtrack) {
                QVariantMap m;
                m["id"]           = QString::fromStdString(t.id);
                m["title"]        = QString::fromStdString(t.title);
                m["artist"]       = QString::fromStdString(t.artist);
                m["thumbnailUrl"] = QString::fromStdString(t.thumbnailUrl);
                m["durationSecs"] = t.durationSecs;
                soundtrackList.append(m);
            }
            map["soundtrack"] = soundtrackList;

            QVariantList episodeList;
            for (const auto& ep : info.episodes) {
                QVariantMap m;
                m["season"]    = ep.season;
                m["episode"]   = ep.episode;
                m["title"]     = QString::fromStdString(ep.title);
                m["synopsis"]  = QString::fromStdString(ep.synopsis);
                m["streamUrl"] = QString::fromStdString(ep.streamUrl);
                m["stillUrl"]  = QString::fromStdString(ep.stillUrl);
                episodeList.append(m);
            }
            map["episodes"] = episodeList;

            QMetaObject::invokeMethod(self, [self, map]() {
                if (self) emit self->infoReady(map);
            }, Qt::QueuedConnection);
            return;
        }

        QMetaObject::invokeMethod(self, [self, sourceName]() {
            if (self) emit self->infoError("Source not found: " + sourceName);
        }, Qt::QueuedConnection);
    }).detach();
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
        map["hasTV"]          = caps.hasTV;
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
        for (const auto& url : entry.provider->getSubtitleUrls(id.toStdString()))
            list.append(QString::fromStdString(url));
        break;
    }
    return list;
}

