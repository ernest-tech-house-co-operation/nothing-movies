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

    // Get all registered sources from the aggregator.
    // REQUIREMENT: SearchAggregatorModule must have a method:
    // std::vector<std::pair<std::string, std::shared_ptr<core::ISourceProvider>>> getAllProviders() const;
    auto sources = aggregator_->getAllSources(); // We'll add this method (see note below)

for (const auto& entry : sources) {
    auto caps = entry.provider->getCapabilities();
    if (caps.hasHomepage) {
        auto items = entry.provider->getHomepage();
        QVariantList qmlItems;
        for (const auto& item : items) {
            QVariantMap map;
            map["id"]          = QString::fromStdString(item.id);
            map["title"]       = QString::fromStdString(item.title);
            map["posterUrl"]   = QString::fromStdString(item.imageUrl);
            map["category"]    = QString::fromStdString(item.category);
            map["trailerLink"] = QString::fromStdString(item.trailerLink);
            map["genre"]       = QString::fromStdString(item.genre);
            map["rating"]      = item.rating;
            qmlItems.append(map);
        }
        emit homepageReady(qmlItems, QString::fromStdString(entry.name), false);
        return;
    }
}

    // No source has homepage → fallback to TMDB trending
    if (tmdbBridge_) {
        // Use a one‑shot connection to get the trending data.
        // If tmdbBridge_ already has cached data, we could call a getter;
        // for simplicity, we request it and emit when ready.
        auto* bridge = tmdbBridge_;
        connect(bridge, &TmdbBridge::trendingReady, this, [this, bridge](QVariantList movies) {
            // Ensure we only emit once by disconnecting after first emission
            disconnect(bridge, &TmdbBridge::trendingReady, this, nullptr);
            emit homepageReady(movies, "TMDB Fallback", true);
        });
        bridge->loadTrending(); // This may be called twice if already loaded, but it's okay.
    } else {
        emit homepageError("No TMDB bridge available for fallback");
    }
}

} // namespace ui