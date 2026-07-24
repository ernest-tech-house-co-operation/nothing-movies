#include "ui/TmdbBridge.h"
#include <QMetaObject>
#include <thread>

namespace ui {

TmdbBridge::TmdbBridge(const std::string& apiKey, QObject* parent)
    : QObject(parent), client_(std::make_shared<tmdb_client::TmdbClient>(apiKey)) {}

QVariantList TmdbBridge::toVariantList(const std::vector<tmdb_client::TmdbListItem>& items) {
    QVariantList list;
    for (const auto& item : items) {
        QVariantMap m;
        m["tmdbId"] = item.tmdbId;
        m["title"] = QString::fromStdString(item.title);
        m["posterUrl"] = QString::fromStdString(item.posterUrl);
        m["backdropUrl"] = QString::fromStdString(item.backdropUrl);
        m["year"] = item.year;
        list.append(m);
    }
    return list;
}

// Network calls block, so each load runs off the UI thread. Result is
// marshalled back via QueuedConnection so QML only ever touches it on
// the main thread. QPointer guards against the bridge being destroyed
// mid-request (e.g. app closing while a request is in flight).
void TmdbBridge::loadTrending() {
    auto client = client_;
    QPointer<TmdbBridge> self(this);
    std::thread([client, self]() {
        auto items = client->getTrending();
        auto list = TmdbBridge::toVariantList(items);
        if (!self) return;
        QMetaObject::invokeMethod(self, [self, list]() {
            if (self) emit self->trendingReady(list);
        }, Qt::QueuedConnection);
    }).detach();
}

void TmdbBridge::loadUpcoming() {
    auto client = client_;
    QPointer<TmdbBridge> self(this);
    std::thread([client, self]() {
        auto items = client->getUpcoming();
        auto list = TmdbBridge::toVariantList(items);
        if (!self) return;
        QMetaObject::invokeMethod(self, [self, list]() {
            if (self) emit self->upcomingReady(list);
        }, Qt::QueuedConnection);
    }).detach();
}

}